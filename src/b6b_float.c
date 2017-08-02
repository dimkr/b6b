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

struct b6b_obj *b6b_float_new(const b6b_float f)
{
	struct b6b_obj *o;

	o = b6b_new();
	if (o) {
		o->f = f;
		o->flags = B6B_TYPE_FLOAT;
	}

	return o;
}

int b6b_as_float(struct b6b_obj *o)
{
	char *p = NULL;

	if (o->flags & B6B_TYPE_FLOAT)
		return 1;

	/* always prefer the string representation over the integer one, since it's
	 * more accurate */
	if (o->flags & B6B_TYPE_STR) {
		if (!o->slen)
			return 0;

		o->f = strtod(o->s, &p);

		/* make sure the entire string was converted - for example, 127\0abc is
		 * an invalid integer */
		if (p != o->s + o->slen)
			return 0;
	} else if (o->flags & B6B_TYPE_LIST) {
		if (!b6b_as_str(o) || !o->slen)
			return 0;

		o->f = strtod(o->s, &p);
		if (p != o->s + o->slen)
			return 0;
	}
	else
		o->f = (b6b_float)o->i;

	o->flags |= B6B_TYPE_FLOAT;
	return 1;
}
