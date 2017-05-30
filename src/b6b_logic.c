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

#include <b6b.h>

static enum b6b_res b6b_logic_proc_not(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &o))
		return b6b_return_bool(interp, !b6b_obj_istrue(o));

	return B6B_ERR;
}

static enum b6b_res b6b_logic_proc_eq(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *a, *b;
	int eq;

	if (!b6b_proc_get_args(interp, args, "ooo", NULL, &a, &b) ||
	    b6b_unlikely(!b6b_obj_eq(a, b, &eq)))
		return B6B_ERR;

	return b6b_return_bool(interp, eq);
}

static enum b6b_res b6b_logic_proc_ne(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *a, *b;
	int eq;

	if (!b6b_proc_get_args(interp, args, "ooo", NULL, &a, &b) ||
	    b6b_unlikely(!b6b_obj_eq(a, b, &eq)))
		return B6B_ERR;

	return b6b_return_bool(interp, !eq);
}

static enum b6b_res b6b_logic_proc_and(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *a, *b;

	if (b6b_proc_get_args(interp, args, "ooo", NULL, &a, &b))
		return b6b_return_bool(interp, b6b_obj_istrue(a) && b6b_obj_istrue(b));

	return B6B_ERR;
}

static enum b6b_res b6b_logic_proc_or(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *a, *b;

	if (b6b_proc_get_args(interp, args, "ooo", NULL, &a, &b))
		return b6b_return_bool(interp, b6b_obj_istrue(a) || b6b_obj_istrue(b));

	return B6B_ERR;
}

static enum b6b_res b6b_logic_proc_xor(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *a, *b;

	if (b6b_proc_get_args(interp, args, "ooo", NULL, &a, &b) == 3)
		return b6b_return_bool(interp, b6b_obj_istrue(a) ^ b6b_obj_istrue(b));

	return B6B_ERR;
}

static enum b6b_res b6b_logic_proc_if(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *b, *t, *f;
	unsigned int argc = b6b_proc_get_args(interp,
	                                      args,
	                                      "ooo|o",
	                                      NULL,
	                                      &b,
	                                      &t,
	                                      &f);

	if (!argc)
		return B6B_ERR;

	if (b6b_obj_istrue(b))
		return b6b_call(interp, t);

	if (argc == 4)
		return b6b_call(interp, f);

	/* no else clause */
	return B6B_OK;
}

static const struct b6b_ext_obj b6b_logic[] = {
	{
		.name = "!",
		.type = B6B_OBJ_STR,
		.val.s = "!",
		.proc = b6b_logic_proc_not
	},
	{
		.name = "==",
		.type = B6B_OBJ_STR,
		.val.s = "==",
		.proc = b6b_logic_proc_eq
	},
	{
		.name = "!=",
		.type = B6B_OBJ_STR,
		.val.s = "!=",
		.proc = b6b_logic_proc_ne
	},
	{
		.name = "&&",
		.type = B6B_OBJ_STR,
		.val.s = "&&",
		.proc = b6b_logic_proc_and
	},
	{
		.name = "||",
		.type = B6B_OBJ_STR,
		.val.s = "||",
		.proc = b6b_logic_proc_or
	},
	{
		.name = "^",
		.type = B6B_OBJ_STR,
		.val.s = "^",
		.proc = b6b_logic_proc_xor
	},
	{
		.name = "if",
		.type = B6B_OBJ_STR,
		.val.s = "if",
		.proc = b6b_logic_proc_if
	}
};
__b6b_ext(b6b_logic);
