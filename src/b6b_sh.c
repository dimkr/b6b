/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2020 Dima Krasner
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <paths.h>
#include <errno.h>

#include <b6b.h>

struct b6b_sh {
	struct b6b_obj *pid;
	int fd;
};

static void b6b_pid_del(void *priv)
{
	pid_t pid = (pid_t)(intptr_t)priv;

	if (pid)
		waitpid(pid, NULL, WNOHANG);
}

static ssize_t b6b_sh_peeksz(struct b6b_interp *interp, void *priv)
{
	const struct b6b_sh *s = (const struct b6b_sh *)priv;

	return b6b_fd_peeksz(interp, (void *)(intptr_t)s->fd);
}

static ssize_t b6b_sh_read(struct b6b_interp *interp,
                           void *priv,
                           unsigned char *buf,
                           const size_t len,
                           int *eof,
                           int *again)
{
	const struct b6b_sh *s = (const struct b6b_sh *)priv;

	return b6b_fd_read(interp, (void *)(intptr_t)s->fd, buf, len, eof, again);
}

static ssize_t b6b_sh_write(struct b6b_interp *interp,
                            void *priv,
                            const unsigned char *buf,
                            const size_t len)
{
	const struct b6b_sh *s = (const struct b6b_sh *)priv;

	return b6b_fd_write(interp, (void *)(intptr_t)s->fd, buf, len);
}

static int b6b_sh_fd(void *priv)
{
	const struct b6b_sh *s = (const struct b6b_sh *)priv;

	return s->fd;
}

static void b6b_sh_close(void *priv)
{
	struct b6b_sh *s = (struct b6b_sh *)priv;

	close(s->fd);
	b6b_unref(s->pid);

	free(priv);
}

static const struct b6b_strm_ops b6b_sh_in_ops = {
	.write = b6b_sh_write,
	.fd = b6b_sh_fd,
	.close = b6b_sh_close
};

static const struct b6b_strm_ops b6b_sh_out_ops = {
	.peeksz = b6b_sh_peeksz,
	.read = b6b_sh_read,
	.fd = b6b_sh_fd,
	.close = b6b_sh_close
};

static struct b6b_obj *b6b_sh_new_pipe(struct b6b_interp *interp,
                                       struct b6b_obj *pid,
                                       const struct b6b_strm_ops *ops,
                                       const char *type,
                                       const int end,
                                       int fds[2])
{
	struct b6b_sh *s;
	struct b6b_obj *o;
	int fl, err;

	s = (struct b6b_sh *)malloc(sizeof(*s));
	if (!b6b_allocated(s))
		return NULL;

	if (pipe2(fds, O_CLOEXEC) < 0) {
		err = errno;
		free(s);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	fl = fcntl(fds[end], F_GETFL);
	if ((fl < 0) || (fcntl(fds[end], F_SETFL, fl | O_NONBLOCK) < 0)) {
		err = errno;
		close(fds[1]);
		close(fds[0]);
		free(s);
		b6b_return_strerror(interp, err);
		return NULL;
	}

	s->fd = fds[end];
	s->pid = b6b_ref(pid);

	o = b6b_strm_fmt(interp, ops, s, type);
	if (b6b_unlikely(!o)) {
		b6b_sh_close(s);
		close(fds[!end]);
		return NULL;
	}

	return o;
}

static enum b6b_res b6b_sh_proc_sh(struct b6b_interp *interp,
                                   struct b6b_obj *args)
{
	struct b6b_obj *shs[3], *pid, *l, *cmd;
	int fds[3][2];

	if (!b6b_proc_get_args(interp, args, "os", NULL, &cmd))
		return B6B_ERR;

	/* we create a special int object whose destructor reaps the zombie process;
	 * each stream holds one reference to it */
	pid = b6b_int_new(0);
	if (b6b_unlikely(!pid))
		return B6B_ERR;

	shs[0] = b6b_sh_new_pipe(interp, pid, &b6b_sh_in_ops, "stdin", 1, fds[0]);
	if (!shs[0]) {
		b6b_destroy(pid);
		return B6B_ERR;
	}

	shs[1] = b6b_sh_new_pipe(interp, pid, &b6b_sh_out_ops, "stdout", 0, fds[1]);
	if (!shs[1]) {
		b6b_destroy(shs[0]);
		b6b_destroy(pid);
		return B6B_ERR;
	}

	shs[2] = b6b_sh_new_pipe(interp, pid, &b6b_sh_out_ops, "stderr", 0, fds[2]);
	if (!shs[2]) {
		b6b_destroy(shs[1]);
		b6b_destroy(shs[0]);
		b6b_destroy(pid);
		return B6B_ERR;
	}

	pid->i = (b6b_int)fork();
	switch (pid->i) {
		case -1:
			b6b_destroy(shs[2]);
			b6b_destroy(shs[1]);
			b6b_destroy(shs[0]);
			b6b_destroy(pid);
			return B6B_ERR;

		case 0:
			if ((dup2(fds[0][0], STDIN_FILENO) == STDIN_FILENO) &&
			    (dup2(fds[1][1], STDOUT_FILENO) == STDOUT_FILENO) &&
			    (dup2(fds[2][1], STDERR_FILENO) == STDERR_FILENO))
				execl(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd->s, (char *)NULL);

			exit(EXIT_FAILURE);
	}

	close(fds[0][0]);
	close(fds[1][1]);
	close(fds[2][1]);

	pid->del = b6b_pid_del;
	pid->priv = (void *)(intptr_t)pid->i;

	l = b6b_list_build(pid, shs[0], shs[1], shs[2], NULL);
	if (b6b_unlikely(!l)) {
		b6b_destroy(shs[2]);
		b6b_destroy(shs[1]);
		b6b_destroy(shs[0]);
		b6b_destroy(pid);
		return B6B_ERR;
	}

	b6b_unref(shs[2]);
	b6b_unref(shs[1]);
	b6b_unref(shs[0]);
	b6b_unref(pid);

	return b6b_return(interp, l);
}

static const struct b6b_ext_obj b6b_sh[] = {
	{
		.name = "sh",
		.type = B6B_TYPE_STR,
		.val.s = "sh",
		.proc = b6b_sh_proc_sh
	}
};
__b6b_ext(b6b_sh);
