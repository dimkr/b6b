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

#include <string.h>

#include "linenoise/linenoise.h"
#include <b6b.h>

#define B6B_LINENOISE_HISTORY_SIZE 64

static enum b6b_res b6b_linenoise_proc_read(struct b6b_interp *interp,
                                            struct b6b_obj *args)
{
	struct b6b_obj *p, *o;
	char *s;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &p))
		return B6B_ERR;

	s = linenoise(p->s);
	if (s) {
		o = b6b_str_new(s, strlen(s));
		if (b6b_likely(o))
			return b6b_return(interp, o);

		/* we assume linenoiseFree() calls free(), since we don't use a custom
		 * allocator */
		linenoiseFree(s);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_linenoise_proc_add(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *s;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	return linenoiseHistoryAdd(s->s) ? B6B_OK : B6B_ERR;
}

static enum b6b_res b6b_linenoise_proc_save(struct b6b_interp *interp,
                                            struct b6b_obj *args)
{
	struct b6b_obj *s;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	return (linenoiseHistorySave(s->s) == 0) ? B6B_OK : B6B_ERR;
}

static enum b6b_res b6b_linenoise_proc_load(struct b6b_interp *interp,
                                            struct b6b_obj *args)
{
	struct b6b_obj *s;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	return (linenoiseHistoryLoad(s->s) == 0) ? B6B_OK : B6B_ERR;
}

static const struct b6b_ext_obj b6b_linenoise[] = {
	{
		.name = "linenoise.read",
		.type = B6B_TYPE_STR,
		.val.s = "linenoise.read",
		.proc = b6b_linenoise_proc_read
	},
	{
		.name = "linenoise.add",
		.type = B6B_TYPE_STR,
		.val.s = "linenoise.add",
		.proc = b6b_linenoise_proc_add
	},
	{
		.name = "linenoise.save",
		.type = B6B_TYPE_STR,
		.val.s = "linenoise.save",
		.proc = b6b_linenoise_proc_save
	},
	{
		.name = "linenoise.load",
		.type = B6B_TYPE_STR,
		.val.s = "linenoise.load",
		.proc = b6b_linenoise_proc_load
	}
};
__b6b_ext(b6b_linenoise);

static int b6b_linenoise_init(struct b6b_interp *interp)
{
	return linenoiseHistorySetMaxLen(B6B_LINENOISE_HISTORY_SIZE);
}
__b6b_init(b6b_linenoise_init);
