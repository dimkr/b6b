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
#include <stdarg.h>

#include <b6b.h>

unsigned int b6b_proc_get_args(struct b6b_interp *interp,
                               struct b6b_obj *l,
                               const char *fmt,
                               ...)
{
	va_list ap;
	struct b6b_litem *li;
	unsigned int argc;

	va_start(ap, fmt);
	argc = b6b_list_vparse(l, fmt, ap);
	va_end(ap);

	if (!argc) {
		li = b6b_list_first(l);

		if (b6b_as_str(li->o) && b6b_as_str(l)) {
			b6b_return_fmt(interp,
			               "bad call: '%s', expect '%s%s'",
			               l->s,
			               li->o->s,
			               &fmt[1]);
		}
	}

	return argc;
}

struct b6b_proc {
	struct b6b_obj *body;
	struct b6b_obj *priv;
};

static enum b6b_res b6b_proc_proc(struct b6b_interp *interp,
                                  struct b6b_obj *args)
{
	struct b6b_litem *li = b6b_list_first(args);
	struct b6b_proc *proc = (struct b6b_proc *)li->o->priv;
	enum b6b_res res;

	if (b6b_unlikely(!b6b_frame_set_args(interp, interp->fg->curr, args)) ||
	    b6b_unlikely(!b6b_local(interp, interp->dot, proc->priv)))
		return B6B_ERR;

	res = b6b_call(interp, proc->body);
	if (res == B6B_RET)
		return B6B_OK;

	return res;
}

static void b6b_proc_del(void *priv)
{
	struct b6b_proc *proc = (struct b6b_proc *)priv;

	b6b_unref(proc->priv);
	b6b_unref(proc->body);
	free(priv);
}

static enum b6b_res b6b_proc_proc_proc(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_proc *priv;
	struct b6b_obj *o, *n, *b, *p;
	unsigned int argc = b6b_proc_get_args(interp,
	                                      args,
	                                      "o s o |o",
	                                      NULL,
	                                      &n,
	                                      &b,
	                                      &p);

	if (!argc)
		return B6B_ERR;

	priv = (struct b6b_proc *)malloc(sizeof(*priv));
	if (b6b_unlikely(!priv))
		return B6B_ERR;

	o = b6b_str_copy(n->s, n->slen);
	if (b6b_unlikely(!o)) {
		free(priv);
		return B6B_ERR;
	}

	if (b6b_unlikely(!b6b_global(interp, n, o))) {
		b6b_destroy(o);
		free(priv);
		return B6B_ERR;
	}

	priv->body = b6b_ref(b);
	priv->priv = (argc == 4) ? b6b_ref(p) : b6b_ref(interp->null);

	o->priv = priv;
	o->proc = b6b_proc_proc;
	o->del = b6b_proc_del;

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_proc[] = {
	{
		.name = "proc",
		.type = B6B_OBJ_STR,
		.val.s = "proc",
		.proc = b6b_proc_proc_proc
	}
};
__b6b_ext(b6b_proc);
