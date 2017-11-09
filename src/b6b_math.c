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

#include <math.h>
#include <errno.h>
#ifdef B6B_HAVE_FENV
#	include <fenv.h>
#endif

#include <b6b.h>

static enum b6b_res b6b_math_proc_add(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_float(interp, m->f + n->f);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_sub(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_float(interp, m->f - n->f);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_mul(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_float(interp, m->f * n->f);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_div(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n)) {
		if (b6b_unlikely(n->f == 0)) {
			b6b_return_str(interp, "/ by 0", sizeof("/ by 0") - 1);
			return B6B_ERR;
		}

		return b6b_return_float(interp, m->f / n->f);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_mod(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;
	double p;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n)) {
		if (b6b_unlikely(n->f == 0)) {
			b6b_return_str(interp, "% by 0", sizeof("% by 0") - 1);
			return B6B_ERR;
		}

#ifndef B6B_HAVE_FENV
		if (isinf(m->f) && !isnan(n->f))
			return B6B_ERR;
#else
		errno = 0;
		feclearexcept(FE_ALL_EXCEPT);
#endif
		p = remainder(m->f, n->f);
#ifdef B6B_HAVE_FENV
		if (errno || fetestexcept(FE_INVALID))
			return B6B_ERR;
#endif

		return b6b_return_float(interp, (b6b_float)p);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_and(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "oii", NULL, &m, &n))
		return b6b_return_int(interp, m->i & n->i);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_or(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "oii", NULL, &m, &n))
		return b6b_return_int(interp, m->i | n->i);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_lt(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_bool(interp, m->f < n->f);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_le(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_bool(interp, m->f <= n->f);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_gt(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_bool(interp, m->f > n->f);

	return B6B_ERR;
}

static enum b6b_res b6b_math_proc_ge(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *m, *n;

	if (b6b_proc_get_args(interp, args, "off", NULL, &m, &n))
		return b6b_return_bool(interp, m->f >= n->f);

	return B6B_ERR;
}

static const struct b6b_ext_obj b6b_math[] = {
	{
		.name = "+",
		.type = B6B_TYPE_STR,
		.val.s = "+",
		.proc = b6b_math_proc_add
	},
	{
		.name = "-",
		.type = B6B_TYPE_STR,
		.val.s = "-",
		.proc = b6b_math_proc_sub
	},
	{
		.name = "*",
		.type = B6B_TYPE_STR,
		.val.s = "*",
		.proc = b6b_math_proc_mul
	},
	{
		.name = "/",
		.type = B6B_TYPE_STR,
		.val.s = "/",
		.proc = b6b_math_proc_div
	},
	{
		.name = "%",
		.type = B6B_TYPE_STR,
		.val.s = "%",
		.proc = b6b_math_proc_mod
	},
	{
		.name = "&",
		.type = B6B_TYPE_STR,
		.val.s = "&",
		.proc = b6b_math_proc_and
	},
	{
		.name = "|",
		.type = B6B_TYPE_STR,
		.val.s = "|",
		.proc = b6b_math_proc_or
	},
	{
		.name = "<",
		.type = B6B_TYPE_STR,
		.val.s = "<",
		.proc = b6b_math_proc_lt
	},
	{
		.name = "<=",
		.type = B6B_TYPE_STR,
		.val.s = "<=",
		.proc = b6b_math_proc_le
	},
	{
		.name = ">",
		.type = B6B_TYPE_STR,
		.val.s = ">",
		.proc = b6b_math_proc_gt
	},
	{
		.name = ">=",
		.type = B6B_TYPE_STR,
		.val.s = ">=",
		.proc = b6b_math_proc_ge
	}
};
__b6b_ext(b6b_math);
