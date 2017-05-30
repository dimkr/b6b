/*
 * This file is part of b6b.
 *
 * Copyright 2017 Dima Krasner
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
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <paths.h>
#include <errno.h>

#include <b6b.h>

struct b6b_sh {
	int fd;
	pid_t pid;
};

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

	return b6b_fd_recv(interp, (void *)(intptr_t)s->fd, buf, len, eof, again);
}

static ssize_t b6b_sh_write(struct b6b_interp *interp,
                            void *priv,
                            const unsigned char *buf,
                            const size_t len)
{
	const struct b6b_sh *s = (const struct b6b_sh *)priv;

	return b6b_fd_send(interp, (void *)(intptr_t)s->fd, buf, len);
}

static int b6b_sh_fd(void *priv)
{
	const struct b6b_sh *s = (const struct b6b_sh *)priv;

	return s->fd;
}

static void b6b_sh_close(void *priv)
{
	struct b6b_sh *s = (struct b6b_sh *)priv;

	waitpid(s->pid, NULL, WNOHANG);
	close(s->fd);

	free(priv);
}

static const struct b6b_strm_ops b6b_sh_ops = {
	.peeksz = b6b_sh_peeksz,
	.read = b6b_sh_read,
	.write = b6b_sh_write,
	.fd = b6b_sh_fd,
	.close = b6b_sh_close
};

static enum b6b_res b6b_sh_proc_sh(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *cmd, *o;
	struct b6b_sh *s;
	int fds[2], err;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &cmd))
		return B6B_ERR;

	s = (struct b6b_sh *)malloc(sizeof(*s));
	if (b6b_unlikely(!s))
		return B6B_ERR;

	/* we redirect both stdin and stdout to a Unix socket, so the stream we
	 * create can be polled for both input and output */
	if (socketpair(AF_UNIX,
	               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
	               0,
	               fds) < 0) {
		err = errno;
		free(s);
		return b6b_return_strerror(interp, err);
	}

	s->pid = fork();
	switch (s->pid) {
		case -1:
			close(fds[1]);
			close(fds[0]);
			free(s);
			return B6B_ERR;

		case 0:
			close(fds[1]);

			if ((dup2(fds[0], STDIN_FILENO) == STDIN_FILENO) &&
			    (dup2(fds[0], STDOUT_FILENO) == STDOUT_FILENO))
				execl(_PATH_BSHELL, _PATH_BSHELL, "-c", cmd->s, (char *)NULL);

			exit(EXIT_FAILURE);
	}

	s->fd = fds[1];
	close(fds[0]);

	o = b6b_strm_fmt(interp, &b6b_sh_ops, s, "exec");
	if (b6b_unlikely(!o)) {
		b6b_sh_close(s);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_sh[] = {
	{
		.name = "sh",
		.type = B6B_OBJ_STR,
		.val.s = "sh",
		.proc = b6b_sh_proc_sh
	}
};
__b6b_ext(b6b_sh);
