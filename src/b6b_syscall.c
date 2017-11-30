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

#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdarg.h>
#ifdef B6B_HAVE_VALGRIND
#	include <valgrind/helgrind.h>
#endif

#include <b6b.h>

static void b6b_do_syscall(void *arg)
{
	struct b6b_syscall_data *data = (struct b6b_syscall_data *)arg;

	data->rval = syscall(data->args[0],
	                     data->args[1],
	                     data->args[2],
	                     data->args[3],
	                     data->args[4],
	                     data->args[5]);
	if (data->rval < 0)
		data->rerrno = errno;
}

int b6b_syscall(struct b6b_interp *interp,
                int *ret,
                const long nr,
                ...)
{
	struct b6b_syscall_data data;
	va_list ap;
	int i;

	va_start(ap, nr);

	data.args[0] = nr;
	for (i = 1; i < sizeof(data.args) / sizeof(data.args[0]); ++i)
		data.args[i] = va_arg(ap, long);

	va_end(ap);

#ifdef B6B_HAVE_VALGRIND
	VALGRIND_HG_DISABLE_CHECKING(&data, sizeof(data));
#endif
	if (!b6b_offload(interp, b6b_do_syscall, &data))
		return 0;

	*ret = data.rval;
	if (data.rval < 0)
		errno = data.rerrno;

	return 1;
}
