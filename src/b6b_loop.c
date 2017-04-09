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

static enum b6b_res b6b_loop_proc_map(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *k, *l, *b, *r;
	struct b6b_litem *li;
	enum b6b_res res;

	if (!b6b_proc_get_args(interp, args, "o o l o", NULL, &k, &l, &b))
		return B6B_ERR;

	r = b6b_list_new();
	if (b6b_unlikely(!r))
		return B6B_ERR;

	li = b6b_list_first(l);
	while (li) {
		if (b6b_unlikely(!b6b_local(interp, k, li->o))) {
			b6b_destroy(r);
			return B6B_ERR;
		}

		res = b6b_call(interp, b);
		if (res != B6B_OK) {
			b6b_destroy(r);
			return res;
		}

		if (b6b_unlikely(!b6b_list_add(r, interp->fg->_))) {
			b6b_destroy(r);
			return B6B_ERR;
		}

		li = b6b_list_next(li);
	}


	return b6b_return(interp, r);
}

static enum b6b_res b6b_loop_proc_while(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *c, *b;
	enum b6b_res res = B6B_OK;

	if (!b6b_proc_get_args(interp, args, "o o o", NULL, &c, &b))
		return B6B_ERR;

	do {
		res = b6b_eval(interp, c);
		if ((res != B6B_OK) || !b6b_obj_istrue(interp->fg->_))
			break;

		res = b6b_call(interp, b);
	} while (res == B6B_OK);

	return res;
}

static enum b6b_res b6b_loop_proc_range(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *start, *end, *stepo, *l, *n;
	b6b_num i, step = 1;

	switch (b6b_proc_get_args(interp,
	                          args,
	                          "o n n |n",
	                          NULL,
	                          &start,
	                          &end,
	                          &stepo)) {
		case 4:
			step = stepo->n;

		case 3:
			if (start->n > end->n)
				return B6B_ERR;
			break;

		default:
			return B6B_ERR;
	}


	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return B6B_ERR;

	for (i = start->n; i <= end->n; i += step) {
		n = b6b_num_new(i);
		if (b6b_unlikely(!n)) {
			b6b_destroy(l);
			return B6B_ERR;
		}

		if (b6b_unlikely(!b6b_list_add(l, n))) {
			b6b_destroy(n);
			b6b_destroy(l);
			return B6B_ERR;
		}

		b6b_unref(n);
	}

	return b6b_return(interp, l);
}

static const struct b6b_ext_obj b6b_loop[] = {
	{
		.name = "map",
		.type = B6B_OBJ_STR,
		.val.s = "map",
		.proc = b6b_loop_proc_map
	},
	{
		.name = "while",
		.type = B6B_OBJ_STR,
		.val.s = "while",
		.proc = b6b_loop_proc_while
	},
	{
		.name = "range",
		.type = B6B_OBJ_STR,
		.val.s = "range",
		.proc = b6b_loop_proc_range
	}
};
__b6b_ext(b6b_loop);
