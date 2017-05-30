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

#include <stdlib.h>
#include <math.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>

#include <b6b.h>

static const struct b6b_strm_ops b6b_timer_ops = {
	.peeksz = b6b_fd_peeksz_u64,
	.read = b6b_fd_read_u64,
	.fd = b6b_fd_fd,
	.close = b6b_fd_close
};

static enum b6b_res b6b_timer_proc_timer(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct itimerspec its;
	struct b6b_obj *i, *o;
	int fd, err;

	if (!b6b_proc_get_args(interp, args, "on", NULL, &i))
		return B6B_ERR;

	fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
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
