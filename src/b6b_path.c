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
#include <string.h>
#include <errno.h>

#include <b6b.h>

static enum b6b_res b6b_path_proc_realpath(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *s, *o;
	char *abs;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	abs = realpath(s->s, NULL);
	if (!abs)
		return b6b_return_strerror(interp, errno);

	o = b6b_str_new(abs, strlen(abs));
	if (b6b_unlikely(!o)) {
		free(abs);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_path[] = {
	{
		.name = "realpath",
		.type = B6B_OBJ_STR,
		.val.s = "realpath",
		.proc = b6b_path_proc_realpath
	}
};
__b6b_ext(b6b_path);
