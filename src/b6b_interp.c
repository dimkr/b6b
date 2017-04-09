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

/* for GNU strerror_r() */
#define _GNU_SOURCE

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#undef _GNU_SOURCE

#include <b6b.h>

static void b6b_thread_routine(const int th,
                               const int tl,
                               const int ih,
                               const int il)
{
	struct b6b_interp *interp = (struct b6b_interp *)(uintptr_t)il;
	struct b6b_thread *t = (struct b6b_thread *)(uintptr_t)tl;

	if (sizeof(int) < sizeof(uintptr_t)) {
#define INTBITS (sizeof(int) * 8)
		interp = (struct b6b_interp *)(uintptr_t)((uint64_t)ih << INTBITS | il);
		t = (struct b6b_thread *)(uintptr_t)((uint64_t)th << INTBITS | tl);
	}

	b6b_call(interp, t->fn);
	t->flags |= B6B_THREAD_DONE;
}

static int b6b_interp_new_ext_obj(struct b6b_interp *interp,
                                  const struct b6b_ext_obj *eo)
{
	struct b6b_obj *v, *k = b6b_str_copy(eo->name, strlen(eo->name));

	if (b6b_unlikely(!k))
		return 0;

	switch (eo->type) {
		case B6B_OBJ_STR:
			v = b6b_str_copy(eo->val.s, strlen(eo->val.s));
			break;

		case B6B_OBJ_NUM:
			v = b6b_num_new(eo->val.n);
			break;

		default:
			b6b_destroy(k);
			return 0;
	}

	if (b6b_unlikely(!v))
		return 0;

	if (eo->proc)
		v->proc = eo->proc;

	if (eo->del)
		v->del = eo->del;

	if (b6b_unlikely(!b6b_global(interp, k, v))) {
		b6b_destroy(v);
		b6b_destroy(k);
		return 0;
	}

	b6b_unref(v);
	b6b_unref(k);
	return 1;
}

int b6b_interp_new(struct b6b_interp *interp,
                   const int argc,
                   const char *argv[])
{
	struct b6b_obj *args, *a;
	const struct b6b_ext *e;
	b6b_initf *ip;
	int i;
	unsigned int j;

	args = b6b_list_new();
	if (b6b_unlikely(!args))
		return 0;

	for (i = 0; i < argc; ++i) {
		a = b6b_str_copy(argv[i], strlen(argv[i]));
		if (b6b_unlikely(!a)) {
			b6b_destroy(args);
			return 0;
		}

		if (b6b_unlikely(!b6b_list_add(args, a))) {
			b6b_destroy(a);
			b6b_destroy(args);
			return 0;
		}

		b6b_unref(a);
	}

	memset(interp, 0, sizeof(*interp));

	interp->stksiz = sysconf(_SC_THREAD_ATTR_STACKSIZE);
	if (interp->stksiz <= 0)
		goto bail;

	interp->null = b6b_str_copy("null", 4);
	if (b6b_unlikely(!interp->null))
		goto bail;

	interp->zero = b6b_num_new(0);
	if (b6b_unlikely(!interp->zero))
		goto bail;

	interp->one = b6b_num_new(1);
	if (b6b_unlikely(!interp->one))
		goto bail;

	interp->dot = b6b_str_copy(".", 1);
	if (b6b_unlikely(!interp->dot))
		goto bail;

	interp->at = b6b_str_copy("@", 1);
	if (b6b_unlikely(!interp->at))
		goto bail;

	interp->_ = b6b_str_copy("_", 1);
	if (b6b_unlikely(!interp->_))
		goto bail;

	interp->global = b6b_frame_new(NULL);
	if (b6b_unlikely(!interp->global) ||
	    b6b_unlikely(!b6b_frame_start(interp, interp->global, args)))
		goto bail;

	if (!b6b_thread_init(&interp->threads[0],
	                     NULL,
	                     NULL,
	                     interp->global,
	                     interp->null,
	                     b6b_thread_routine,
	                     interp,
	                     (size_t)interp->stksiz))
		goto bail;

	interp->fg = &interp->threads[0];
	interp->qstep = 0;

	for (e = b6b_ext_first; e < b6b_ext_last; ++e) {
		for (j = 0; j < e->n; ++j) {
			if (!b6b_interp_new_ext_obj(interp, &e->os[j]))
				goto bail;
		}
	}

	for (ip = b6b_init_first; ip < b6b_init_last; ++ip) {
		if (!(*ip)(interp))
			goto bail;
	}

	b6b_unref(args);
	return 1;

bail:
	b6b_interp_destroy(interp);
	b6b_unref(args);
	return 0;
}

void b6b_interp_destroy(struct b6b_interp *interp)
{
	int i;

	for (i = B6B_NTHREADS - 1; i >= 0; --i)
		b6b_thread_destroy(&interp->threads[i]);

	if (interp->global)
		b6b_frame_destroy(interp->global);

	if (interp->_)
		b6b_unref(interp->_);

	if (interp->at)
		b6b_unref(interp->at);

	if (interp->dot)
		b6b_unref(interp->dot);

	if (interp->one)
		b6b_unref(interp->one);

	if (interp->zero)
		b6b_unref(interp->zero);

	if (interp->null)
		b6b_unref(interp->null);
}

enum b6b_res b6b_return_num(struct b6b_interp *interp, const b6b_num n)
{
	struct b6b_obj *o = b6b_num_new(n);

	if (o)
		return b6b_return(interp, o);

	return B6B_ERR;
}

enum b6b_res b6b_return_str(struct b6b_interp *interp,
                            const char *s,
                            const size_t len)
{
	struct b6b_obj *o = b6b_str_copy(s, len);

	if (o)
		return b6b_return(interp, o);

	return B6B_ERR;
}

enum b6b_res b6b_return_fmt(struct b6b_interp *interp, const char *fmt, ...)
{
	va_list ap;
	struct b6b_obj *s;

	va_start(ap, fmt);
	s = b6b_str_vfmt(fmt, ap);
	va_end(ap);
	if (b6b_unlikely(!s))
		return B6B_ERR;

	return b6b_return(interp, s);
}

enum b6b_res b6b_return_strerror(struct b6b_interp *interp, const int err)
{
	static char buf[128];
	const char *s;
	struct b6b_obj *o;

	s = strerror_r(err, buf, sizeof(buf));
	if (s) {
		o = b6b_str_copy(s, strlen(s));
		if (o) {
			b6b_unref(interp->fg->_);
			interp->fg->_ = o;
		}
	}

	return B6B_ERR;
}

struct b6b_obj *b6b_get(struct b6b_interp *interp, const char *name)
{
	struct b6b_frame *f = interp->fg->curr;
	struct b6b_obj *o;

	do {
		if (b6b_dict_get(f->locals, name, &o))
			return o;

		f = f->prev;
	} while (f);

	b6b_return_fmt(interp, "no such obj: %s", name);
	return NULL;
}

static enum b6b_res b6b_stmt_call(struct b6b_interp *, struct b6b_obj *);

enum b6b_res b6b_eval(struct b6b_interp *interp, struct b6b_obj *exp)
{
	struct b6b_obj *o, *stmt;
	enum b6b_res res;

	if (!b6b_as_str(exp))
		return B6B_ERR;

	if (exp->s[0] == '$') {
		o = b6b_get(interp, &exp->s[1]);
		if (o)
			return b6b_return(interp, b6b_ref(o));

		return B6B_ERR;
	}
	else if ((exp->s[0] == '[') && (exp->s[exp->slen - 1] == ']')) {
		stmt = b6b_str_copy(&exp->s[1], exp->slen - 2);
		if (b6b_unlikely(!stmt))
			return B6B_ERR;

		res = b6b_stmt_call(interp, stmt);
		b6b_unref(stmt);
		return res;
	}

	return b6b_return(interp, b6b_ref(exp));
}

static enum b6b_res b6b_on_res(struct b6b_interp *interp,
                               const enum b6b_res res)
{
	/* update _ of the calling frame */
	if (b6b_unlikely(!b6b_local(interp, interp->_, interp->fg->_)))
		return B6B_ERR;

	/* switch to another thread upon B6B_YIELD or after each batch of
	 * B6B_QUANT_LEN statements */
	switch (res) {
		case B6B_YIELD:
			b6b_yield(interp);
			return B6B_OK;

		case B6B_EXIT:
			return res;

		default:
			if (++interp->qstep == B6B_QUANT_LEN) {
				interp->qstep = 0;
				b6b_yield(interp);
			}
	}

	return res;
}

static enum b6b_res b6b_stmt_call(struct b6b_interp *interp,
                                  struct b6b_obj *stmt)
{
	struct b6b_litem *li;
	struct b6b_obj *args;
	enum b6b_res res = B6B_ERR;

	if (b6b_unlikely(!b6b_frame_push(interp)))
		goto out;

	if (!b6b_as_list(stmt))
		goto pop;

	args = b6b_list_new();
	if (b6b_unlikely(!args))
		goto pop;

	b6b_list_foreach(stmt, li) {
		res = b6b_eval(interp, li->o);
		if ((res != B6B_OK) ||
		    b6b_unlikely(!b6b_list_add(args, interp->fg->_))) {
			goto destroy;
		}
	}

	if (!b6b_frame_start(interp, interp->fg->curr, args))
		goto destroy;

	/* reset the return value after argument evaluation */
	b6b_unref(interp->fg->_);
	interp->fg->_ = b6b_ref(interp->null);

	res = b6b_list_first(args)->o->proc(interp, args);

destroy:
	b6b_unref(args);

pop:
	b6b_frame_pop(interp);

out:
	return b6b_on_res(interp, res);
}

enum b6b_res b6b_call(struct b6b_interp *interp, struct b6b_obj *stmts)
{
	struct b6b_litem *li;
	enum b6b_res res;

	if (!b6b_as_list(stmts))
		return B6B_ERR;

	b6b_list_foreach(stmts, li) {
		if (!b6b_as_str(li->o))
			return B6B_ERR;

		if (li->o->slen && (li->o->s[0] != '#')) {
			res = b6b_stmt_call(interp, li->o);
			if (res != B6B_OK)
				return res;
		}
	}

	return B6B_OK;
}

int b6b_start(struct b6b_interp *interp, struct b6b_obj *stmts)
{
	int i;

	for (i = B6B_NTHREADS - 1; i >= 0; --i) {
		if (interp->threads[i].flags & B6B_THREAD_DONE)
			b6b_thread_reset(&interp->threads[i]);

		if (!interp->threads[i].flags) {
			if (b6b_thread_init(&interp->threads[i],
			                    &interp->threads[0],
			                    stmts,
			                    interp->global,
			                    interp->null,
			                    b6b_thread_routine,
			                    interp,
			                    (size_t)interp->stksiz))
				return 1;

			return 0;
		}
	}

	return 0;
}

enum b6b_res b6b_source(struct b6b_interp *interp, const char *path)
{
	struct stat stbuf;
	char *s;
	struct b6b_obj *stmts;
	ssize_t len;
	int fd;
	enum b6b_res res = B6B_ERR;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return B6B_ERR;

	if ((fstat(fd, &stbuf) < 0) || (stbuf.st_size >= SSIZE_MAX)) {
		close(fd);
		return B6B_ERR;
	}

	s = (char *)malloc((size_t)stbuf.st_size + 1);
	if (b6b_unlikely(!s)) {
		close(fd);
		return B6B_ERR;
	}

	len = read(fd, s, (size_t)stbuf.st_size);
	close(fd);
	if (len != (ssize_t)stbuf.st_size) {
		free(s);
		return B6B_ERR;
	}
	s[stbuf.st_size] = '\0';

	stmts = b6b_str_new(s, (size_t)stbuf.st_size);
	if (b6b_unlikely(!stmts)) {
		free(s);
		return B6B_ERR;
	}

	res = b6b_call(interp, stmts);
	b6b_unref(stmts);
	return res;
}

static enum b6b_res b6b_interp_proc_nop(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	return B6B_OK;
}

static enum b6b_res b6b_interp_proc_yield(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	return B6B_YIELD;
}

static enum b6b_res b6b_interp_proc_return(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *o;

	switch (b6b_proc_get_args(interp, args, "o |o", NULL, &o)) {
		case 2:
			b6b_unref(interp->fg->_);
			interp->fg->_ = b6b_ref(o);

		case 1:
			return B6B_RET;
	}

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_continue(struct b6b_interp *interp,
                                             struct b6b_obj *args)
{
	return B6B_CONT;
}

static enum b6b_res b6b_interp_proc_break(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	return B6B_BREAK;
}

static enum b6b_res b6b_interp_proc_throw(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (b6b_proc_get_args(interp, args, "o |o", NULL, &o) == 2) {
		b6b_unref(interp->fg->_);
		interp->fg->_ = b6b_ref(o);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_exit(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *o;

	switch (b6b_proc_get_args(interp, args, "o |o", NULL, &o)) {
		case 2:
			b6b_return(interp, b6b_ref(o));

		case 1:
			return B6B_EXIT;
	}

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_local(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	struct b6b_obj *k, *v;

	if (b6b_proc_get_args(interp, args, "o o o", NULL, &k, &v) &&
	    b6b_dict_set(interp->fg->curr->prev ?
	                 interp->fg->curr->prev->locals :
	                 interp->fg->curr->locals,
	                 k,
	                 v))
		return B6B_OK;

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_global(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *k, *v;

	if (b6b_proc_get_args(interp, args, "o o o", NULL, &k, &v) &&
	    b6b_global(interp, k, v))
		return B6B_OK;

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_export(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *k, *v;

	switch (b6b_proc_get_args(interp, args, "o s |o", NULL, &k, &v)) {
		case 3:
			break;

		case 2:
			v = b6b_get(interp, k->s);
			if (v)
				break;

		default:
			return B6B_ERR;
	}

	if (b6b_dict_set(interp->fg->curr->prev->locals, k, v))
		return B6B_OK;

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_eval(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{


	struct b6b_obj *exp;

	if (b6b_proc_get_args(interp, args, "o o", NULL, &exp))
		return b6b_eval(interp, exp);

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_call(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{


	struct b6b_obj *stmts;

	if (b6b_proc_get_args(interp, args, "o o", NULL, &stmts))
		return b6b_call(interp, stmts);

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc(struct b6b_interp *interp,
                                    struct b6b_obj *args)
{
	struct b6b_obj *i, *s, *o;
	struct b6b_interp *interp2;
	enum b6b_res res;

	if (!b6b_proc_get_args(interp, args, "o s o", &i, &s, &o) ||
	    (strcmp(o->s, "eval") != 0))
		return B6B_ERR;

	interp2 = (struct b6b_interp *)i->priv;
	res = b6b_call(interp2, o);
	b6b_unref(interp->fg->_);
	interp->fg->_ = b6b_ref(interp2->fg->_);
	return res;
}

static void b6b_interp_del(void *priv)
{
	b6b_interp_destroy((struct b6b_interp *)priv);
	free(priv);
}

static enum b6b_res b6b_interp_proc_interp(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *o;
	struct b6b_interp *interp2 = (struct b6b_interp *)malloc(sizeof(*interp2));

	if (b6b_unlikely(!interp2))
		return B6B_ERR;

	o = b6b_str_fmt("interp:%"PRIxPTR, (uintptr_t)interp2);
	if (b6b_unlikely(!o)) {
		free(interp2);
		return B6B_ERR;
	}

	if (!b6b_interp_new(interp2, 0, NULL)) {
		b6b_interp_destroy(interp2);
		free(interp2);
		b6b_destroy(o);
		return B6B_ERR;
	}

	o->priv = interp2;
	o->proc = b6b_interp_proc;
	o->del = b6b_interp_del;
	return b6b_return(interp, o);
}

static enum b6b_res b6b_interp_proc_spawn(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (b6b_proc_get_args(interp, args, "o o", NULL, &o) &&
	    b6b_start(interp, o))
		return B6B_OK;

	return B6B_ERR;
}

static const struct b6b_ext_obj b6b_interp[] = {
	{
		.name = "nop",
		.type = B6B_OBJ_STR,
		.val.s = "nop",
		.proc = b6b_interp_proc_nop
	},
	{
		.name = "yield",
		.type = B6B_OBJ_STR,
		.val.s = "yield",
		.proc = b6b_interp_proc_yield
	},
	{
		.name = "return",
		.type = B6B_OBJ_STR,
		.val.s = "return",
		.proc = b6b_interp_proc_return
	},
	{
		.name = "continue",
		.type = B6B_OBJ_STR,
		.val.s = "continue",
		.proc = b6b_interp_proc_continue
	},
	{
		.name = "break",
		.type = B6B_OBJ_STR,
		.val.s = "break",
		.proc = b6b_interp_proc_break
	},
	{
		.name = "throw",
		.type = B6B_OBJ_STR,
		.val.s = "throw",
		.proc = b6b_interp_proc_throw
	},
	{
		.name = "exit",
		.type = B6B_OBJ_STR,
		.val.s = "exit",
		.proc = b6b_interp_proc_exit
	},
	{
		.name = "local",
		.type = B6B_OBJ_STR,
		.val.s = "local",
		.proc = b6b_interp_proc_local
	},
	{
		.name = "global",
		.type = B6B_OBJ_STR,
		.val.s = "global",
		.proc = b6b_interp_proc_global
	},
	{
		.name = "export",
		.type = B6B_OBJ_STR,
		.val.s = "export",
		.proc = b6b_interp_proc_export
	},
	{
		.name = "eval",
		.type = B6B_OBJ_STR,
		.val.s = "eval",
		.proc = b6b_interp_proc_eval
	},
	{
		.name = "call",
		.type = B6B_OBJ_STR,
		.val.s = "call",
		.proc = b6b_interp_proc_call
	},
	{
		.name = "interp.new",
		.type = B6B_OBJ_STR,
		.val.s = "interp.new",
		.proc = b6b_interp_proc_interp
	},
	{
		.name = "spawn",
		.type = B6B_OBJ_STR,
		.val.s = "spawn",
		.proc = b6b_interp_proc_spawn
	}
};
__b6b_ext(b6b_interp);
