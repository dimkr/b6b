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
#include <assert.h>

#include <b6b.h>

int main()
{
	struct b6b_obj *o, *s;

	o = b6b_int_new(0);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_int_new(2);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_float_new(0);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_float_new(2);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_list_new();
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	s = b6b_str_copy("1", 0);
	assert(s);
	o = b6b_list_build(s, NULL);
	b6b_unref(s);
	assert(o);
	assert(b6b_obj_istrue(o));
	assert(b6b_as_str(o));
	assert(b6b_obj_istrue(o));
	b6b_unref(b6b_list_pop(o, b6b_list_first(o)));
	assert(b6b_list_empty(o));
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("", 0);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("a", 1);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("0", 1);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("1", 1);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("00", 2);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("0.0", 3);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("001", 3);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("0.000000001", 11);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy(".0", 2);
	assert(o);
	assert(!b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy(".1", 2);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	o = b6b_str_copy("1.0", 3);
	assert(o);
	assert(b6b_obj_istrue(o));
	b6b_unref(o);

	return EXIT_SUCCESS;
}
