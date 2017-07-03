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
#include <math.h>
#include <fenv.h>

#include <b6b.h>

struct b6b_obj *b6b_int_new(const b6b_int i)
{
	struct b6b_obj *o;

	o = b6b_new();
	if (o) {
		o->i = i;
		o->f = (b6b_float)i;
		o->flags = B6B_TYPE_INT | B6B_TYPE_FLOAT;
	}

	return o;
}

int b6b_as_int(struct b6b_obj *o)
{
	if (o->flags & B6B_TYPE_INT)
		return 1;

	if (!b6b_as_float(o))
		return 0;

	feclearexcept(FE_ALL_EXCEPT);
	o->i = (b6b_int)floor(o->f);
	if (fetestexcept(FE_INVALID))
		return 0;

	o->flags |= B6B_TYPE_INT;
	return 1;
}
