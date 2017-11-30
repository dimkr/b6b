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
	struct b6b_obj *ks, *l, *b, *r;
	struct b6b_litem *li, *kli;
	enum b6b_res res;

	if (!b6b_proc_get_args(interp, args, "ollo", NULL, &ks, &l, &b))
		return B6B_ERR;

	if (b6b_list_empty(ks))
		return B6B_ERR;

	r = b6b_list_new();
	if (b6b_unlikely(!r))
		return B6B_ERR;

	li = b6b_list_first(l);
	while (li) {
		kli = b6b_list_first(ks);
		while (kli) {
			if (!li) {
				b6b_destroy(r);
				b6b_return_str(interp,
				               "not enough items",
				               sizeof("not enough items") - 1);
				return B6B_ERR;
			}

			if (b6b_unlikely(!b6b_local(interp, kli->o, li->o))) {
				b6b_destroy(r);
				return B6B_ERR;
			}

			li = b6b_list_next(li);
			kli = b6b_list_next(kli);
		}

		res = b6b_call(interp, b);
		switch (res) {
			case B6B_OK:
				if (b6b_unlikely(!b6b_list_add(r, interp->fg->_))) {
					b6b_destroy(r);
					return B6B_ERR;
				}

			/* ignore the current iteration's return value on B6B_CONT, to allow
			 * long-running loops without increase in memory usage */
			case B6B_CONT:
				break;

			case B6B_BREAK:
				goto done;

			default:
				b6b_destroy(r);
				return res;
		}
	}

done:
	return b6b_return(interp, r);
}

static enum b6b_res b6b_loop_proc_loop(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *b;
	enum b6b_res res = B6B_OK;

	if (!b6b_proc_get_args(interp, args, "oo", NULL, &b))
		return B6B_ERR;

	do {
		res = b6b_call(interp, b);
		if (res == B6B_BREAK)
			return B6B_OK;
	} while (res == B6B_OK);

	return res;
}

static enum b6b_res b6b_loop_proc_range(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *start, *end, *stepo, *l, *io;
	b6b_int i, step = 1;

	switch (b6b_proc_get_args(interp,
	                          args,
	                          "oii|i",
	                          NULL,
	                          &start,
	                          &end,
	                          &stepo)) {
		case 4:
			step = stepo->i;
			if (step <= 0)
				return B6B_ERR;

		case 3:
			if (start->i > end->i)
				return B6B_ERR;
			break;

		default:
			return B6B_ERR;
	}

	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return B6B_ERR;

	for (i = start->i; i <= end->i; i += step) {
		io = b6b_int_new(i);
		if (b6b_unlikely(!io)) {
			b6b_destroy(l);
			return B6B_ERR;
		}

		if (b6b_unlikely(!b6b_list_add(l, io))) {
			b6b_destroy(io);
			b6b_destroy(l);
			return B6B_ERR;
		}

		b6b_unref(io);
	}

	return b6b_return(interp, l);
}

static const struct b6b_ext_obj b6b_loop[] = {
	{
		.name = "map",
		.type = B6B_TYPE_STR,
		.val.s = "map",
		.proc = b6b_loop_proc_map
	},
	{
		.name = "loop",
		.type = B6B_TYPE_STR,
		.val.s = "loop",
		.proc = b6b_loop_proc_loop
	},
	{
		.name = "range",
		.type = B6B_TYPE_STR,
		.val.s = "range",
		.proc = b6b_loop_proc_range
	}
};
__b6b_ext(b6b_loop);
