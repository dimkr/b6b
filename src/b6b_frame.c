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

#include <b6b.h>

struct b6b_frame *b6b_frame_new(struct b6b_frame *prev)
{
	struct b6b_frame *f = (struct b6b_frame *)malloc(sizeof(*f));

	if (b6b_likely(f)) {
		f->locals = b6b_dict_new();
		if (b6b_unlikely(!f->locals)) {
			free(f);
			return NULL;
		}

		f->prev = prev;
	}

	return f;
}

void b6b_frame_destroy(struct b6b_frame *f)
{
	b6b_unref(f->locals);
	free(f);
}

struct b6b_frame *b6b_frame_push(struct b6b_interp *interp)
{
	struct b6b_frame *f;

	if (b6b_unlikely(interp->fg->depth >= B6B_MAX_NESTING)) {
		b6b_return_str(interp,
		               "call stack overflow",
		               sizeof("call stack overflow") - 1);
		return NULL;
	}

	f = b6b_frame_new(interp->fg->curr);
	if (b6b_likely(f)) {
		interp->fg->curr = f;
		++interp->fg->depth;
	}

	return f;
}

int b6b_frame_start(struct b6b_interp *interp,
                    struct b6b_frame *f,
                    struct b6b_obj *args)
{
	struct b6b_litem *li;
	struct b6b_obj *n;
	b6b_num i = 0;

	if (!b6b_as_list(args) || !b6b_dict_set(f->locals, interp->at, args))
		return 0;

	li = b6b_list_first(args);
	do {
		n = b6b_num_new(i);
		if (b6b_unlikely(!n))
			return 0;

		if (b6b_unlikely(!b6b_dict_set(f->locals, n, li->o))) {
			b6b_destroy(n);
			return 0;
		}
		b6b_unref(n);

		li = b6b_list_next(li);
		if (!li)
			break;

		++i;
	} while (1);

	return 1;
}

void b6b_frame_pop(struct b6b_interp *interp)
{
	struct b6b_frame *prev = interp->fg->curr->prev;

	b6b_frame_destroy(interp->fg->curr);

	interp->fg->curr = prev;
	--interp->fg->depth;
}
