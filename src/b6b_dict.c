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

#include <string.h>
#include <b6b.h>

int b6b_dict_set(struct b6b_obj *d, struct b6b_obj *k, struct b6b_obj *v)
{
	struct b6b_litem *kli, *vli;
	int eq;

	kli = b6b_list_first(d);
	while (kli) {
		if (b6b_unlikely(!b6b_obj_eq(kli->o, k, &eq)))
			return 0;

		vli = b6b_list_next(kli);
		if (!vli)
			return 0;

		if (eq) {
			b6b_unref(vli->o);
			vli->o = b6b_ref(v);
			return 1;
		}

		kli = b6b_list_next(vli);
	}

	if (b6b_unlikely(!b6b_list_add(d, k)))
		return 0;

	if (b6b_unlikely(!b6b_list_add(d, v))) {
		b6b_unref(b6b_list_pop(d));
		return 0;
	}

	return 1;
}

int b6b_dict_get(struct b6b_obj *d, const char *kn, struct b6b_obj **v)
{
	struct b6b_litem *k, *vli;
	uint32_t h = b6b_str_hash(kn, strlen(kn));

	k = b6b_list_first(d);
	while (k) {
		if (!b6b_obj_hash(k->o))
			return 0;

		vli = b6b_list_next(k);
		if (!vli)
			return 0;

		if (k->o->hash == h) {
			*v = vli->o;
			return 1;
		}

		k = b6b_list_next(vli);
	}

	return 0;
}
