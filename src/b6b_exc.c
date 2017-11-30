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
#ifdef B6B_HAVE_THREADS
	int exit;
#endif
	enum b6b_res res, eres = B6B_ERR, fres;

	argc = b6b_proc_get_args(interp, args, "oo|oo", NULL, &t, &e, &f);
	if (!argc)
		return B6B_ERR;

	res = b6b_call(interp, t);
	if (res == B6B_ERR) {
		if (interp->opts & B6B_OPT_RAISE) {
			if (argc == 2)
				return res;

			eres = b6b_call(interp, e);

			if (eres != B6B_EXIT)
				eres = B6B_ERR;
		}
		else {
			/* silence the error thrown by the try block if there's no except
			 * block */
			if (argc == 2)
				return B6B_OK;

			eres = b6b_call(interp, e);
		}
	}

	if (argc == 4) {
#ifdef B6B_HAVE_THREADS
		exit = interp->exit;
		/* if the try block triggered exit, postpone it and let the finally
		 * block run */
		interp->exit = 0;
#endif
		o = b6b_ref(interp->fg->_);
		fres = b6b_call(interp, f);
		b6b_unref(interp->fg->_);
		interp->fg->_ = o;
#ifdef B6B_HAVE_THREADS
		interp->exit = exit;
#endif

		if ((res == B6B_RET) || (res == B6B_EXIT))
			return res;

		if ((res == B6B_ERR) && (eres != B6B_OK))
			return eres;

		return fres;
	}

	if (res == B6B_ERR)
		return eres;

	return res;
}

static const struct b6b_ext_obj b6b_exc[] = {
	{
		.name = "try",
		.type = B6B_TYPE_STR,
		.val.s = "try",
		.proc = b6b_exc_proc_try
	}
};
__b6b_ext(b6b_exc);
