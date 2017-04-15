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

#include <sys/signalfd.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>

#include <b6b.h>

struct b6b_signal {
	int fd;
	sigset_t set;
};

static ssize_t b6b_signal_read(struct b6b_interp *interp,
                               void *priv,
                               unsigned char *buf,
                               const size_t len,
                               int *eof,
                               int *again)
{
	struct signalfd_siginfo si;
	const struct b6b_signal *s = (const struct b6b_signal *)priv;
	ssize_t out;
	int outc;

	out = read(s->fd, &si, sizeof(si));
	if (out == sizeof(si)) {
		outc = snprintf((char *)buf, len, "%"PRIu32, si.ssi_signo);
		if ((outc >= len) || (outc < 0))
			return -1;

		out = (ssize_t)outc;
		*again = 0;
	} else if (!out)
		*again = 0;
	else {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return 0;

		b6b_return_strerror(interp, errno);
		return -1;
	}

	return out;
}

static int b6b_signal_fd(void *priv)
{
	const struct b6b_signal *s = (const struct b6b_signal *)priv;

	return s->fd;
}

static void b6b_signal_close(void *priv)
{
	struct b6b_signal *s = (struct b6b_signal *)priv;

	sigprocmask(SIG_UNBLOCK, &s->set, NULL);
	close(s->fd);

	free(priv);
}

static const struct b6b_strm_ops b6b_signal_ops = {
	.read = b6b_signal_read,
	.fd = b6b_signal_fd,
	.close = b6b_signal_close
};

static enum b6b_res b6b_signal_proc_signal(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_litem *li;
	struct b6b_signal *sig;
	struct b6b_strm *strm;
	struct b6b_obj *o;
	int err;

	/* must specify at least one signal */
	li = b6b_list_next(b6b_list_first(args));
	if (!li)
		return B6B_ERR;

	sig = (struct b6b_signal *)malloc(sizeof(*sig));
	if (b6b_unlikely(!sig))
		return B6B_ERR;

	if (sigemptyset(&sig->set) < 0) {
		free(sig);
		return B6B_ERR;
	}

	do {
		if (!b6b_as_num(li->o) ||
		    (li->o->n < 1) ||
		    (li->o->n > INT_MAX) ||
		    (sigaddset(&sig->set, (int)li->o->n) < 0)) {
			free(sig);
			return B6B_ERR;
		}

		li = b6b_list_next(li);
	} while (li);

	if (sigprocmask(SIG_BLOCK, &sig->set, NULL) < 0) {
		err = errno;
		free(sig);
		return b6b_return_strerror(interp, err);
	}

	sig->fd = signalfd(-1, &sig->set, SFD_NONBLOCK);
	if (sig->fd < 0) {
		err = errno;
		free(sig);
		return b6b_return_strerror(interp, err);
	}

	strm = (struct b6b_strm *)malloc(sizeof(*strm));
	if (b6b_unlikely(!strm)) {
		close(sig->fd);
		free(sig);
		return B6B_ERR;
	}

	strm->ops = &b6b_signal_ops;
	strm->flags = 0;
	strm->priv = sig;

	o = b6b_strm_fmt(interp, strm, "signal");
	if (b6b_unlikely(!o)) {
		b6b_strm_destroy(strm);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_signal[] = {
	{
		.name = "SIGINT",
		.type = B6B_OBJ_NUM,
		.val.n = SIGINT
	},
	{
		.name = "SIGTERM",
		.type = B6B_OBJ_NUM,
		.val.n = SIGTERM
	},
	{
		.name = "SIGHUP",
		.type = B6B_OBJ_NUM,
		.val.n = SIGHUP
	},
	{
		.name = "SIGALRM",
		.type = B6B_OBJ_NUM,
		.val.n = SIGALRM
	},
	{
		.name = "SIGSTOP",
		.type = B6B_OBJ_NUM,
		.val.n = SIGSTOP
	},
	{
		.name = "SIGCONT",
		.type = B6B_OBJ_NUM,
		.val.n = SIGCONT
	},
	{
		.name = "SIGIO",
		.type = B6B_OBJ_NUM,
		.val.n = SIGIO
	},
	{
		.name = "SIGUSR1",
		.type = B6B_OBJ_NUM,
		.val.n = SIGUSR1
	},
	{
		.name = "SIGUSR2",
		.type = B6B_OBJ_NUM,
		.val.n = SIGUSR2
	},
	{
		.name = "signal",
		.type = B6B_OBJ_STR,
		.val.s = "signal",
		.proc = b6b_signal_proc_signal
	}
};
__b6b_ext(b6b_signal);