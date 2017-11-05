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

#include <time.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <b6b.h>

static enum b6b_res b6b_time_proc_sleep(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct timespec req, rem;
	struct b6b_obj *f;
	int ret;

	if (!b6b_proc_get_args(interp, args, "of", NULL, &f))
		return B6B_ERR;

	/* we assume sizeof(time_t) == 4 */
	if ((f->f <= 0) || (f->f > UINT_MAX))
		return B6B_ERR;

	/* we want to avoid costly offloading the nanosleep() call, if the interval
	 * is zero */
	if (!f->f)
		return B6B_OK;

	req.tv_sec = (time_t)floor(f->f);
	req.tv_nsec = labs((long)(1000000000 * (f->f - (b6b_float)req.tv_sec)));

	do {
		if (!b6b_syscall(interp, &ret, __NR_nanosleep, &req, &rem))
			return B6B_ERR;

		if (ret == 0)
			break;

		if (errno != EINTR)
			return b6b_return_strerror(interp, errno);

		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	} while (1);

	return B6B_OK;
}

static const struct b6b_ext_obj b6b_time[] = {
	{
		.name = "sleep",
		.type = B6B_TYPE_STR,
		.val.s = "sleep",
		.proc = b6b_time_proc_sleep
	}
};
__b6b_ext(b6b_time);
