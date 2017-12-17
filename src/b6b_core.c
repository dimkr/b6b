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

static enum b6b_res b6b_core_proc_nop(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	return B6B_OK;
}

static enum b6b_res b6b_core_proc_echo(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (!b6b_proc_get_args(interp, args, "oo", NULL, &o))
		return B6B_ERR;

	return b6b_return(interp, b6b_ref(o));
}

static enum b6b_res b6b_core_proc_yield(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	return B6B_YIELD;
}

static enum b6b_res b6b_core_proc_return(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *o;

	switch (b6b_proc_get_args(interp, args, "o|o", NULL, &o)) {
		case 2:
			b6b_return(interp, b6b_ref(o));

		case 1:
			return B6B_RET;
	}

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_continue(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	return B6B_CONT;
}

static enum b6b_res b6b_core_proc_break(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	return B6B_BREAK;
}

static enum b6b_res b6b_core_proc_throw(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (b6b_proc_get_args(interp, args, "o|o", NULL, &o) == 2) {
		b6b_unref(interp->fg->_);
		interp->fg->_ = b6b_ref(o);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_exit(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *o;

	switch (b6b_proc_get_args(interp, args, "o|o", NULL, &o)) {
		case 2:
			b6b_return(interp, b6b_ref(o));

		case 1:
			return B6B_EXIT;
	}

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_local(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *k, *v;

	if (b6b_proc_get_args(interp, args, "ooo", NULL, &k, &v) &&
	    b6b_dict_set(interp->fg->curr->prev ?
	                 interp->fg->curr->prev->locals :
	                 interp->fg->curr->locals,
	                 k,
	                 v))
		return b6b_return(interp, b6b_ref(v));

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_global(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *k, *v;

	if (b6b_proc_get_args(interp, args, "ooo", NULL, &k, &v) &&
	    b6b_global(interp, k, v))
		return b6b_return(interp, b6b_ref(v));

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_export(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *k, *v;
	const struct b6b_frame *f = interp->fg->curr->prev;

	switch (b6b_proc_get_args(interp, args, "os|o", NULL, &k, &v)) {
		case 3:
			break;

		case 2:
			v = b6b_get(interp, k);
			if (v)
				break;

		default:
			return B6B_ERR;
	}

	if (!f->prev)
		return B6B_ERR;

	if (b6b_dict_set(f->prev->locals, k, v))
		return b6b_return(interp, b6b_ref(v));

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_eval(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{


	struct b6b_obj *exp;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &exp) && b6b_as_str(exp))
		return b6b_eval(interp, exp);

	return B6B_ERR;
}

static enum b6b_res b6b_core_proc_repr(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s, *s2;
	char *buf;
	size_t i, len;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s) ||
	    (s->slen >= SIZE_MAX - 2))
		return B6B_ERR;

	if (s->slen) {
		if ((s->s[0] != '{') || (s->s[s->slen - 1] != '}')) {
			for (i = 0; i < s->slen; ++i) {
				if (!b6b_isspace(s->s[i]))
					continue;

				len = s->slen + 2;

				buf = (char *)malloc(len + 1);
				if (!b6b_allocated(buf))
					return B6B_ERR;

				buf[0] = '{';
				memcpy(&buf[1], s->s, s->slen);
				buf[s->slen + 1] = '}';
				buf[len] = '\0';

				s2 = b6b_str_new(buf, len);
				if (b6b_unlikely(!s2)) {
					free(buf);
					return B6B_ERR;
				}

				return b6b_return(interp, s2);
			}
		}

		return b6b_return(interp, b6b_ref(s));
	}

	return b6b_return_str(interp, "{}", 2);
}

static enum b6b_res b6b_core_proc_call(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *stmts;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &stmts))
		return b6b_call(interp, stmts);

	return B6B_ERR;
}

#ifdef B6B_HAVE_THREADS

static enum b6b_res b6b_core_proc_spawn(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &o) &&
	    b6b_start(interp, o))
		return B6B_OK;

	return B6B_ERR;
}

#endif

static enum b6b_res b6b_core_proc_source(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *path;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &path) || !path->slen)
		return B6B_ERR;

	return b6b_source(interp, path->s);
}


static const struct b6b_ext_obj b6b_core[] = {
	{
		.name = "nop",
		.type = B6B_TYPE_STR,
		.val.s = "nop",
		.proc = b6b_core_proc_nop
	},
	{
		.name = "echo",
		.type = B6B_TYPE_STR,
		.val.s = "echo",
		.proc = b6b_core_proc_echo
	},
	{
		.name = "yield",
		.type = B6B_TYPE_STR,
		.val.s = "yield",
		.proc = b6b_core_proc_yield
	},
	{
		.name = "return",
		.type = B6B_TYPE_STR,
		.val.s = "return",
		.proc = b6b_core_proc_return
	},
	{
		.name = "continue",
		.type = B6B_TYPE_STR,
		.val.s = "continue",
		.proc = b6b_core_proc_continue
	},
	{
		.name = "break",
		.type = B6B_TYPE_STR,
		.val.s = "break",
		.proc = b6b_core_proc_break
	},
	{
		.name = "throw",
		.type = B6B_TYPE_STR,
		.val.s = "throw",
		.proc = b6b_core_proc_throw
	},
	{
		.name = "exit",
		.type = B6B_TYPE_STR,
		.val.s = "exit",
		.proc = b6b_core_proc_exit
	},
	{
		.name = "local",
		.type = B6B_TYPE_STR,
		.val.s = "local",
		.proc = b6b_core_proc_local
	},
	{
		.name = "global",
		.type = B6B_TYPE_STR,
		.val.s = "global",
		.proc = b6b_core_proc_global
	},
	{
		.name = "export",
		.type = B6B_TYPE_STR,
		.val.s = "export",
		.proc = b6b_core_proc_export
	},
	{
		.name = "eval",
		.type = B6B_TYPE_STR,
		.val.s = "eval",
		.proc = b6b_core_proc_eval
	},
	{
		.name = "repr",
		.type = B6B_TYPE_STR,
		.val.s = "repr",
		.proc = b6b_core_proc_repr
	},
	{
		.name = "call",
		.type = B6B_TYPE_STR,
		.val.s = "call",
		.proc = b6b_core_proc_call
	},
#ifdef B6B_HAVE_THREADS
	{
		.name = "spawn",
		.type = B6B_TYPE_STR,
		.val.s = "spawn",
		.proc = b6b_core_proc_spawn
	},
#endif
	{
		.name = "source",
		.type = B6B_TYPE_STR,
		.val.s = "source",
		.proc = b6b_core_proc_source
	}
};
__b6b_ext(b6b_core);
