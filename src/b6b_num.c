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

struct b6b_obj *b6b_num_new(const b6b_num n)
{
	struct b6b_obj *o;

	o = b6b_new();
	if (o) {
		o->n = n;
		o->flags |= B6B_OBJ_NUM;
	}

	return o;
}

int b6b_as_num(struct b6b_obj *o)
{
	char *p = NULL;

	if (!(o->flags & B6B_OBJ_NUM)) {
		if (!b6b_as_str(o))
			return 0;

		o->n = strtod(o->s, &p);
		if (*p)
			return 0;

		o->flags |= B6B_OBJ_NUM;
	}

	return 1;
}
