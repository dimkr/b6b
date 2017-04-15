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

#include <b6b.h>

static int b6b_timer_fd(void *priv)
{
	return (int)(intptr_t)priv;
}

static void b6b_timer_close(void *priv)
{
	close((int)(intptr_t)priv);
}

static const struct b6b_strm_ops b6b_timer_ops = {
	.fd = b6b_timer_fd,
	.close = b6b_timer_close
};

static enum b6b_res b6b_timer_proc_timer(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct itimerspec its;
	struct b6b_obj *i, *o;
	struct b6b_strm *s;
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

	s = (struct b6b_strm *)malloc(sizeof(*s));
	if (b6b_unlikely(!s)) {
		close(fd);
		return B6B_ERR;
	}

	s->ops = &b6b_timer_ops;
	s->flags = 0;
	s->priv = (void *)(intptr_t)fd;

	o = b6b_strm_fmt(interp, s, "timer");
	if (b6b_unlikely(!o)) {
		b6b_strm_destroy(s);
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