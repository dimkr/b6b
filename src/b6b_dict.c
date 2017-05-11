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
		b6b_unref(b6b_list_pop(d, NULL));
		return 0;
	}

	return 1;
}

int b6b_dict_get(struct b6b_obj *d, struct b6b_obj *k, struct b6b_obj **v)
{
	struct b6b_litem *kli, *vli;
	int eq;

	kli = b6b_list_first(d);
	while (kli) {
		vli = b6b_list_next(kli);
		if (!vli || !b6b_obj_eq(kli->o, k, &eq))
			return 0;

		if (eq) {
			*v = vli->o;
			return 1;
		}

		kli = b6b_list_next(vli);
	}

	return 0;
}

int b6b_dict_unset(struct b6b_obj *d, struct b6b_obj *k)
{
	struct b6b_litem *li, *vli;
	int eq;

	li = b6b_list_first(d);
	while (li) {
		vli = b6b_list_next(li);
		if (!vli || !b6b_obj_eq(li->o, k, &eq))
			return 0;

		if (eq) {
			b6b_unref(b6b_list_pop(d, li));
			b6b_unref(b6b_list_pop(d, vli));
			return 1;
		}

		li = b6b_list_next(vli);
	}

	return 0;
}

static enum b6b_res b6b_dict_proc_get(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *d, *k, *v, *f = NULL;

	if (!b6b_proc_get_args(interp, args, "o l s |o", NULL, &d, &k, &f))
		return B6B_ERR;

	if (b6b_dict_get(d, k, &v))
		return b6b_return(interp, b6b_ref(v));

	if (f)
		return b6b_return(interp, b6b_ref(f));

	return B6B_ERR;
}

static enum b6b_res b6b_dict_proc_set(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *d, *k, *v;

	if (!b6b_proc_get_args(interp, args, "o l o o", NULL, &d, &k, &v) ||
	    b6b_unlikely(!b6b_dict_set(d, k, v)))
		return B6B_ERR;

	return B6B_OK;
}

static enum b6b_res b6b_dict_proc_unset(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *d, *k;

	if (!b6b_proc_get_args(interp, args, "o l o", NULL, &d, &k) ||
	    !b6b_dict_unset(d, k))
		return B6B_ERR;

	return B6B_OK;
}

static const struct b6b_ext_obj b6b_dict[] = {
	{
		.name = "dict.get",
		.type = B6B_OBJ_STR,
		.val.s = "dict.get",
		.proc = b6b_dict_proc_get
	},
	{
		.name = "dict.set",
		.type = B6B_OBJ_STR,
		.val.s = "dict.set",
		.proc = b6b_dict_proc_set
	},
	{
		.name = "dict.unset",
		.type = B6B_OBJ_STR,
		.val.s = "dict.unset",
		.proc = b6b_dict_proc_unset
	}
};
__b6b_ext(b6b_dict);
