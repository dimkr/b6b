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
#include <string.h>

#include <b6b.h>

int main()
{
	struct b6b_obj *o, *s;

	o = b6b_int_new(1337);
	assert(o);

	assert(o->flags & B6B_TYPE_FLOAT);
	assert(b6b_as_float(o));
	assert(o->flags & B6B_TYPE_FLOAT);
	assert(o->f == 1337);

	assert(b6b_as_str(o));
	assert(strcmp(o->s, "1337") == 0);

	assert(b6b_as_list(o));
	assert(!b6b_list_empty(o));
	assert(b6b_as_str(b6b_list_first(o)->o));
	assert(strcmp(b6b_list_first(o)->o->s, "1337") == 0);

	s = b6b_str_copy("1338", 4);
	assert(s);

	assert(o->flags != B6B_TYPE_LIST);
	assert(b6b_list_add(o, s));
	assert(o->flags == B6B_TYPE_LIST);
	b6b_unref(b6b_list_pop(o, b6b_list_first(o)));

	assert(b6b_as_int(o));
	assert(o->i == 1338);

	s = b6b_float_new(1339.333);
	assert(s);

	assert(b6b_list_add(o, s));
	b6b_unref(b6b_list_pop(o, b6b_list_first(o)));

	assert(o->flags == B6B_TYPE_LIST);
	assert(b6b_as_int(o));
	assert(o->i == 1339);
	assert(b6b_as_float(o));
	assert(o->f == 1339.333);

	return EXIT_SUCCESS;
}
