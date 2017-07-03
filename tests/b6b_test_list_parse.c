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
	struct b6b_obj *a, *b, *c, *d, *e, *f, *args;
	struct b6b_obj *oa, *ob, *oc, *od, *oe, *of;
	struct b6b_litem *li = NULL;

	a = b6b_int_new(4);
	assert(a);

	b = b6b_str_copy("20", 2);
	assert(b);

	c = b6b_float_new(1.333);
	assert(c);

	d = b6b_list_new();
	assert(d);

	e = b6b_list_build(d, NULL);
	assert(e);
	assert(b6b_list_add(e, d));

	f = b6b_int_new(1337);
	assert(f);

	args = b6b_list_build(a, b, c, d, e, f, NULL);
	assert(args);

	b6b_unref(a);
	b6b_unref(b);
	b6b_unref(c);
	b6b_unref(d);
	b6b_unref(e);
	b6b_unref(f);

	oa = ob = oc = od = oe = of = NULL;
	assert(b6b_list_parse(args, "isflli", &oa, &ob, &oc, &od, &oe, &of) == 6);
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(of == f);

	oa = ob = oc = od = oe = of = NULL;
	assert(b6b_list_parse(args, "fiissf", &oa, &ob, &oc, &od, &oe, &of) == 6);
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(of == f);

	oa = ob = oc = od = oe = of = NULL;
	assert(b6b_list_parse(args, "isflli", &oa, NULL, &oc, &od, NULL, &of) == 6);
	assert(oa == a);
	assert(!ob);
	assert(oc == c);
	assert(od == d);
	assert(!oe);
	assert(of == f);

	oa = ob = oc = od = oe = of = NULL;
	assert(b6b_list_parse(args, "isfll|i", &oa, &ob, &oc, &od, &oe, &of) == 6);
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(of == f);

	oa = ob = oc = od = oe = of = NULL;
	assert(b6b_list_parse(args, "isflli|*", &oa, &ob, &oc, &od, &oe, &of, &li) == 6);
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(of == f);

	oa = ob = oc = od = oe = of = NULL;
	li = NULL;
	assert(b6b_list_parse(args, "isfll*", &oa, &ob, &oc, &od, &oe, &li) == 6);
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(li->o == f);

	oa = ob = oc = od = oe = of = NULL;
	li = NULL;
	assert(b6b_list_parse(args, "isfll|*", &oa, &ob, &oc, &od, &oe, &li) == 6);
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(li->o == f);

	oa = ob = oc = od = oe = of = NULL;
	assert(!b6b_list_parse(args, "isfll", &oa, &ob, &oc, &od, &oe));
	assert(oa == a);
	assert(ob == b);
	assert(oc == c);
	assert(od == d);
	assert(oe == e);
	assert(!of);

	oa = c;
	ob = d;
	assert(!b6b_list_parse(args, "is", &oa, NULL));
	assert(oa == a);
	assert(ob == d);

	oa = c;
	ob = d;
	assert(!b6b_list_parse(args, "is", NULL, NULL));
	assert(oa == c);
	assert(ob == d);

	li = NULL;
	assert(b6b_list_parse(args, "isf*", &oa, &ob, &oc, &li) == 4);
	assert(li->o == d);
	assert(b6b_list_next(li)->o == e);
	assert(b6b_list_next(b6b_list_next(li))->o == f);
	assert(!b6b_list_next(b6b_list_next(b6b_list_next(li))));

	li = NULL;
	assert(b6b_list_parse(args, "*", &li) == 1);
	assert(li->o == a);
	assert(b6b_list_next(li)->o == b);

	b6b_unref(args);
	return EXIT_SUCCESS;
}
