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

#include <b6b.h>

static enum b6b_res b6b_exc_proc_try(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *t, *e, *f, *o;
	unsigned int argc;
	enum b6b_res res, eres = B6B_ERR, fres;

	argc = b6b_proc_get_args(interp, args, "o o |o o", NULL, &t, &e, &f);
	if (!argc)
		return B6B_ERR;

	res = b6b_call(interp, t);
	o = b6b_ref(interp->fg->_);
	if (res == B6B_ERR) {
		/* silence the error thrown by the try block if there's no except
		 * block */
		if (argc == 2)
			return B6B_OK;

		eres = b6b_call(interp, e);
		if (eres == B6B_ERR) {
			b6b_unref(o);
			o = b6b_ref(interp->fg->_);
		}
	}

	if (argc == 4) {
		fres = b6b_call(interp, f);

		b6b_unref(interp->fg->_);
		interp->fg->_ = o;

		return fres;
	}

	b6b_unref(interp->fg->_);
	interp->fg->_ = o;

	if (res == B6B_ERR)
		return eres;

	return res;
}

static const struct b6b_ext_obj b6b_exc[] = {
	{
		.name = "try",
		.type = B6B_OBJ_STR,
		.val.s = "try",
		.proc = b6b_exc_proc_try
	}
};
__b6b_ext(b6b_exc);
