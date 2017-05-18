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

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <b6b.h>

static ssize_t b6b_timer_peeksz(struct b6b_interp *interp, void *priv)
{
	return sizeof("18446744073709551615");
}

static ssize_t b6b_timer_read(struct b6b_interp *interp,
                              void *priv,
                              unsigned char *buf,
                              const size_t len,
                              int *eof,
                              int *again)
{
	uint64_t u;
	ssize_t out;
	int outc;

	out = b6b_fd_read(interp, priv, (unsigned char *)&u, sizeof(u), eof, again);
	if (out <= 0)
		return out;

	if (out != sizeof(u))
		return -1;

	outc = snprintf((char *)buf, len, "%"PRIu64, u);
	if ((outc >= len) || (outc < 0))
		return -1;

	*again = 0;
	return (ssize_t)outc;
}

static const struct b6b_strm_ops b6b_timer_ops = {
	.peeksz = b6b_timer_peeksz,
	.read = b6b_timer_read,
	.fd = b6b_fd_fd,
	.close = b6b_fd_close
};

static enum b6b_res b6b_timer_proc_timer(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct itimerspec its;
	struct b6b_obj *i, *o;
	int fd, err;

	if (!b6b_proc_get_args(interp, args, "o n", NULL, &i))
		return B6B_ERR;

	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd < 0)
		return b6b_return_strerror(interp, errno);

	its.it_value.tv_sec = (time_t)floor(i->n);
	its.it_value.tv_nsec = labs(
	                (long)(1000000000 * (i->n - (b6b_num)its.it_value.tv_sec)));
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;
	if (timerfd_settime(fd, 0, &its, NULL) < 0) {
		err = errno;
		close(fd);
		return b6b_return_strerror(interp, err);
	}

	o = b6b_strm_fmt(interp, &b6b_timer_ops, (void *)(intptr_t)fd, "timer");
	if (b6b_unlikely(!o)) {
		close(fd);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_timer[] = {
	{
		.name = "timer",
		.type = B6B_OBJ_STR,
		.val.s = "timer",
		.proc = b6b_timer_proc_timer
	}
};
__b6b_ext(b6b_timer);
