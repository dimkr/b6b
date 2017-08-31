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
#ifdef B6B_HAVE_VALGRIND
#	include <valgrind/memcheck.h>
#endif

#include <b6b.h>

static enum b6b_res b6b_obj_proc(struct b6b_interp *interp,
                                 struct b6b_obj *args)
{
	struct b6b_obj *s, *op, *o;

	if (!b6b_proc_get_args(interp, args, "ss", &s, &op))
		return B6B_ERR;

	if (strcmp(op->s, "decode"))
		return B6B_ERR;

	o = b6b_str_decode(s->s, s->slen);
	if (!o)
		return B6B_ERR;

	return b6b_return(interp, o);
}

struct b6b_obj *b6b_new(void)
{
	struct b6b_obj *o = (struct b6b_obj *)malloc(sizeof(*o));

	if (o) {
		o->refc = 1;
		o->proc = b6b_obj_proc;
		o->del = NULL;
	}

	return o;
}

void b6b_destroy_l(struct b6b_obj *o)
{
	struct b6b_litem *li, *tli;

	b6b_list_foreach_safe(o, li, tli) {
		b6b_unref(li->o);
		free(li);
	}

#ifdef B6B_HAVE_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(&o->l, sizeof(o->l));
#endif
}

void b6b_destroy(struct b6b_obj *o)
{
	if (o->flags & B6B_TYPE_LIST)
		b6b_destroy_l(o);

	if (o->flags & B6B_TYPE_STR)
		free(o->s);

	if (o->del)
		o->del(o->priv);

	free(o);
}

int b6b_obj_hash(struct b6b_obj *o)
{
	if (!(o->flags & B6B_OBJ_HASHED)) {
		if (!b6b_as_str(o))
			return 0;

		o->hash = b6b_str_hash(o->s, o->slen);
		o->flags |= B6B_OBJ_HASHED;
	}

	return 1;
}
