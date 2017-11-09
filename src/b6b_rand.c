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
#include <limits.h>

#include <b6b.h>

static enum b6b_res b6b_rand_proc_choice(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *l;
	struct b6b_litem *li;
	int i, c;

	if (!b6b_proc_get_args(interp, args, "ol", NULL, &l))
		return B6B_ERR;

	li = b6b_list_first(l);
	if (!li)
		return B6B_ERR;

	c = rand_r(&interp->seed);

	for (i = 0; i < c; ++i) {
		li = b6b_list_next(li);
		if (!li) {
			/* if the list length is smaller than the random number, divide the
			 * latter by the former */
			c %= (i + 1);

			li = b6b_list_first(l);
			for (i = 1; i <= c; ++i)
				li = b6b_list_next(li);

			break;
		}
	}

	return b6b_return(interp, b6b_ref(li->o));
}

static enum b6b_res b6b_rand_proc_randint(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	struct b6b_obj *start, *end;
	unsigned int delta;

	if (!b6b_proc_get_args(interp, args, "oii", NULL, &start, &end) ||
	    (start->i < INT_MIN) ||
	    (start->i >= INT_MAX) ||
	    (end->i <= start->i) ||
	    (end->i > INT_MAX))
		return B6B_ERR;

	delta = (unsigned int)(end->i - start->i);
	if (delta == UINT_MAX)
		return B6B_ERR;

	return b6b_return_int(
	               interp,
	               (b6b_int)(start->i +
	                         ((unsigned int)(rand_r(&interp->seed) & UINT_MAX) %
	                                         (delta + 1))));
}

static const struct b6b_ext_obj b6b_rand[] = {
	{
		.name = "choice",
		.type = B6B_TYPE_STR,
		.val.s = "choice",
		.proc = b6b_rand_proc_choice
	},
	{
		.name = "randint",
		.type = B6B_TYPE_STR,
		.val.s = "randint",
		.proc = b6b_rand_proc_randint
	}
};
__b6b_ext(b6b_rand);
