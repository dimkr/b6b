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
#include <time.h>
#include <stdio.h>

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
		interp = (struct b6b_interp *)(uintptr_t)((uint64_t)ih << INTBITS | (unsigned int)il);
		t = (struct b6b_thread *)(uintptr_t)((uint64_t)th << INTBITS | (unsigned int)tl);
	}

	b6b_call(interp, t->fn);

	/* mark this thread as dead, switch to another and let b6b_wait() free it */
	t->flags |= B6B_THREAD_DONE;
	b6b_yield(interp);
}

static int b6b_interp_new_ext_obj(struct b6b_interp *interp,
                                  const struct b6b_ext_obj *eo)
{
	struct b6b_obj *v, *k = b6b_str_copy(eo->name, strlen(eo->name));

	if (b6b_unlikely(!k))
		return 0;

	switch (eo->type) {
		case B6B_TYPE_STR:
			v = b6b_str_copy(eo->val.s, strlen(eo->val.s));
			break;

		case B6B_TYPE_INT:
			v = b6b_int_new(eo->val.i);
			break;

		default:
			b6b_destroy(k);
			return 0;
	}

	if (b6b_unlikely(!v)) {
		b6b_destroy(k);
		return 0;
	}

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
                   struct b6b_obj *args,
                   const uint8_t opts)
{
	const struct b6b_ext *e;
	b6b_initf *ip;
	unsigned int j;

	b6b_thread_init(&interp->threads);
	interp->fg = NULL;

	/* we allocate a small stack (just two pages) */
	interp->stksiz = sysconf(_SC_PAGESIZE);
	if (interp->stksiz <= 0)
		goto bail;
	interp->stksiz *= 2;

	interp->null = b6b_str_copy("", 0);
	if (b6b_unlikely(!interp->null))
		goto bail;

	interp->zero = b6b_int_new(0);
	if (b6b_unlikely(!interp->zero))
		goto bail;

	interp->one = b6b_int_new(1);
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
	    b6b_unlikely(!b6b_frame_set_args(interp, interp->global, args)))
		goto bail;

	interp->qstep = 0;
	interp->exit = 0;

	interp->fg = b6b_thread_new(&interp->threads,
	                            NULL,
	                            interp->global,
	                            interp->null,
	                            b6b_thread_routine,
	                            interp,
	                            (size_t)interp->stksiz);
	if (!interp->fg)
		goto bail;
	b6b_thread_push(&interp->threads, interp->fg, NULL);

	interp->seed = (unsigned int)time(NULL);
	interp->opts = opts & B6B_OPT_NBF;

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

	interp->opts = opts;

	return 1;

bail:
	b6b_interp_destroy(interp);
	return 0;
}

int b6b_interp_new_argv(struct b6b_interp *interp,
                        const int argc,
                        const char *argv[],
                        const uint8_t opts)
{
	struct b6b_obj *args, *a;
	int i;

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

	if (!b6b_interp_new(interp, args, opts)) {
		b6b_destroy(args);
		return 0;
	}

	b6b_unref(args);
	return 1;
}

static void b6b_join(struct b6b_interp *interp)
{
	struct b6b_thread *t;

	/* wait until all threads except the main thread are inactive */
	interp->exit = 1;
	do {} while (b6b_yield(interp));

	/* destroy the main thread */
	t = b6b_thread_first(&interp->threads);
	if (t)
		b6b_thread_destroy(t);
}

void b6b_interp_destroy(struct b6b_interp *interp)
{
	if (interp->fg)
		b6b_join(interp);

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

enum b6b_res b6b_return_int(struct b6b_interp *interp, const b6b_int i)
{
	struct b6b_obj *o = b6b_int_new(i);

	if (o)
		return b6b_return(interp, o);

	return B6B_ERR;
}

enum b6b_res b6b_return_float(struct b6b_interp *interp, const b6b_float f)
{
	struct b6b_obj *o = b6b_float_new(f);

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

struct b6b_obj *b6b_get(struct b6b_interp *interp, struct b6b_obj *name)
{
	struct b6b_frame *f = interp->fg->curr;
	struct b6b_obj *o;

	if (b6b_as_str(name)) {
		do {
			if (!b6b_dict_get(f->locals, name, &o))
				return NULL;

			if (o)
				return o;

			f = f->prev;
		} while (f);

		b6b_return_fmt(interp, "no such obj: %s", name->s);
	}

	return NULL;
}

static enum b6b_res b6b_stmt_call(struct b6b_interp *, struct b6b_obj *);

enum b6b_res b6b_eval(struct b6b_interp *interp, struct b6b_obj *exp)
{
	struct b6b_obj *name, *o, *stmt;
	enum b6b_res res;

	if (exp->slen) {
		switch (exp->s[0]) {
			case '$':
				name = b6b_str_copy(&exp->s[1], exp->slen - 1);
				if (b6b_unlikely(!name))
					return B6B_ERR;

				o = b6b_get(interp, name);
				b6b_destroy(name);
				if (o)
					return b6b_return(interp, b6b_ref(o));

				return B6B_ERR;

			case '[':
				if ((exp->slen == 1) || (exp->s[exp->slen - 1] != ']')) {
					b6b_return_str(interp,
					               "uneven []",
					               sizeof("uneven []") - 1);
					return B6B_ERR;
				}

				stmt = b6b_str_copy(&exp->s[1], exp->slen - 2);
				if (b6b_unlikely(!stmt))
					return B6B_ERR;

				res = b6b_stmt_call(interp, stmt);
				b6b_unref(stmt);
				return res;
		}
	}

	return b6b_return(interp, b6b_ref(exp));
}

static void b6b_wait(struct b6b_interp *interp)
{
	struct b6b_thread *t, *tt;

	b6b_thread_foreach_safe(&interp->threads, t, tt) {
		if (t->flags & B6B_THREAD_DONE)
			b6b_thread_pop(&interp->threads, t);
	}
}

int b6b_yield(struct b6b_interp *interp)
{
	struct b6b_thread *bg, *t;

#define THREAD_PENDING(t) \
	(t->flags & B6B_THREAD_BG) && !(t->flags & B6B_THREAD_DONE)

	t = b6b_thread_next(interp->fg);
	while (t) {
		if (THREAD_PENDING(t))
			goto swap;

		t = b6b_thread_next(t);
	}

	t = b6b_thread_first(&interp->threads);
	while (t != interp->fg) {
		if (THREAD_PENDING(t))
			goto swap;

		t = b6b_thread_next(t);
	}

	b6b_wait(interp);
	return 0;

swap:
	bg = interp->fg;
	interp->fg = t;
	interp->qstep = 0;
	b6b_thread_swap(bg, interp->fg);
	b6b_wait(interp);
	return 1;
}

static enum b6b_res b6b_on_res(struct b6b_interp *interp,
                               const enum b6b_res res)
{
	if (res == B6B_YIELD) {
		b6b_yield(interp);
		return B6B_OK;
	}

	++interp->qstep;

	/* update _ of the calling frame */
	if (b6b_unlikely(!b6b_local(interp, interp->_, interp->fg->_)))
		return B6B_ERR;

	if (res == B6B_EXIT) {
		/* signal all threads to stop */
		interp->exit = 1;
		return res;
	}

	/* switch to another thread after each batch of B6B_QUANT_LEN statements */
	if (interp->qstep == B6B_QUANT_LEN)
		b6b_yield(interp);

	return res;
}

static enum b6b_res b6b_stmt_call(struct b6b_interp *interp,
                                  struct b6b_obj *stmt)
{
	struct b6b_litem *li;
	struct b6b_frame *f;
	enum b6b_res res = B6B_ERR;

	/* if the interpreter is exiting, issue B6B_EXIT in the current thread;
	 * b6b_join() takes care of sweeping through all threads */
	if (interp->exit)
		return B6B_EXIT;

	f = b6b_frame_push(interp);
	if (b6b_unlikely(!f))
		goto out;

	if (!b6b_as_list(stmt))
		goto pop;

	b6b_list_foreach(stmt, li) {
		if (!b6b_as_str(li->o))
			goto pop;

		res = b6b_eval(interp, li->o);
		if ((res != B6B_OK) ||
		    b6b_unlikely(!b6b_list_add(f->args, interp->fg->_)))
			goto pop;
	}

	if ((interp->opts & B6B_OPT_TRACE) &&
	    (!b6b_as_str(f->args) ||
	     (fwrite("+ ", 2, 1, stderr) != 1) ||
	     (fwrite(f->args->s, f->args->slen, 1, stderr) != 1) ||
	     (fputc('\n', stderr) != '\n')))
		goto pop;

	/* reset the return value after argument evaluation */
	b6b_unref(interp->fg->_);
	interp->fg->_ = b6b_ref(interp->null);

	res = b6b_list_first(f->args)->o->proc(interp, f->args);

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

enum b6b_res b6b_call_copy(struct b6b_interp *interp,
                           const char *s,
                           const size_t len)
{
	struct b6b_obj *o;
	enum b6b_res res;

	o = b6b_str_copy(s, len);
	if (b6b_unlikely(!o))
		return B6B_ERR;

	res = b6b_call(interp, o);
	b6b_unref(o);
	return res;
}

int b6b_start(struct b6b_interp *interp, struct b6b_obj *stmts)
{
	struct b6b_thread *t;

	t = b6b_thread_new(&interp->threads,
	                   stmts,
	                   interp->global,
	                   interp->null,
	                   b6b_thread_routine,
	                   interp,
	                   (size_t)interp->stksiz);
	if (t) {
		b6b_thread_push(&interp->threads, t, interp->fg);
		return 1;
	}

	return 0;
}

enum b6b_res b6b_source(struct b6b_interp *interp, const char *path)
{
	struct stat stbuf;
	char *s, *pos;
	struct b6b_obj *stmts;
	ssize_t len;
	ptrdiff_t off;
	int fd;
	enum b6b_res res = B6B_ERR;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return B6B_ERR;

	if ((fstat(fd, &stbuf) < 0) ||
	    !stbuf.st_size ||
	    (stbuf.st_size >= SSIZE_MAX)) {
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

	/* if the file begins with a shebang, the code starts at the second line */
	if ((s[0] != '#') || (stbuf.st_size < 2) || (s[1] != '!')) {
		stmts = b6b_str_new(s, (size_t)stbuf.st_size);
		if (b6b_unlikely(!stmts)) {
			free(s);
			return B6B_ERR;
		}
	}
	else {
		pos = strchr(s, '\n');
		if (!pos) {
			free(s);
			return B6B_ERR;
		}

		off = pos - s;
		if (off)
			++off;

		stmts = b6b_str_copy(pos, (size_t)(stbuf.st_size - off));
		free(s);
		if (b6b_unlikely(!stmts))
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

	switch (b6b_proc_get_args(interp, args, "o|o", NULL, &o)) {
		case 2:
			b6b_return(interp, b6b_ref(o));

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

	if (b6b_proc_get_args(interp, args, "o|o", NULL, &o) == 2) {
		b6b_unref(interp->fg->_);
		interp->fg->_ = b6b_ref(o);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_exit(struct b6b_interp *interp,
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

static enum b6b_res b6b_interp_proc_local(struct b6b_interp *interp,
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

static enum b6b_res b6b_interp_proc_global(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	struct b6b_obj *k, *v;

	if (b6b_proc_get_args(interp, args, "ooo", NULL, &k, &v) &&
	    b6b_global(interp, k, v))
		return b6b_return(interp, b6b_ref(v));

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_export(struct b6b_interp *interp,
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

static enum b6b_res b6b_interp_proc_eval(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{


	struct b6b_obj *exp;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &exp) && b6b_as_str(exp))
		return b6b_eval(interp, exp);

	return B6B_ERR;
}

static enum b6b_res b6b_interp_proc_repr(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *s, *s2;
	char *buf;
	size_t i, len;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_OK;

	if (s->slen) {
		if ((s->s[0] != '{') || (s->s[s->slen - 1] != '}')) {
			for (i = 0; i < s->slen; ++i) {
				if (!b6b_isspace(s->s[i]))
					continue;

				len = s->slen + 2;

				buf = (char *)malloc(len + 1);
				if (b6b_unlikely(!buf))
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

static enum b6b_res b6b_interp_proc_call(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{


	struct b6b_obj *stmts;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &stmts))
		return b6b_call(interp, stmts);

	return B6B_ERR;
}

static enum b6b_res b6b_b6b_proc(struct b6b_interp *interp,
                                 struct b6b_obj *args)
{
	struct b6b_obj *i, *s, *o;
	struct b6b_interp *interp2;
	enum b6b_res res;

	if (!b6b_proc_get_args(interp, args, "oso", &i, &s, &o) ||
	    (strcmp(s->s, "call") != 0))
		return B6B_ERR;

	interp2 = (struct b6b_interp *)i->priv;

	res = b6b_call(interp2, o);
	b6b_unref(interp->fg->_);
	interp->fg->_ = b6b_ref(interp2->fg->_);
	return res;
}

static void b6b_b6b_del(void *priv)
{
	b6b_interp_destroy((struct b6b_interp *)priv);
	free(priv);
}

static enum b6b_res b6b_interp_proc_b6b(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *o, *l;
	struct b6b_interp *b6b;

	if (!b6b_proc_get_args(interp, args, "ol", NULL, &l))
		return B6B_ERR;

	b6b = (struct b6b_interp *)malloc(sizeof(*b6b));
	if (b6b_unlikely(!b6b))
		return B6B_ERR;

	o = b6b_str_fmt("b6b:%"PRIxPTR, (uintptr_t)b6b);
	if (b6b_unlikely(!o)) {
		free(b6b);
		return B6B_ERR;
	}

	if (!b6b_interp_new(b6b, l, interp->opts)) {
		b6b_b6b_del(b6b);
		b6b_destroy(o);
		return B6B_ERR;
	}

	o->priv = b6b;
	o->proc = b6b_b6b_proc;
	o->del = b6b_b6b_del;
	return b6b_return(interp, o);
}

static enum b6b_res b6b_interp_proc_spawn(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	struct b6b_obj *o;

	if (b6b_proc_get_args(interp, args, "oo", NULL, &o) &&
	    b6b_start(interp, o))
		return B6B_OK;

	return B6B_ERR;
}

static const struct b6b_ext_obj b6b_interp[] = {
	{
		.name = "nop",
		.type = B6B_TYPE_STR,
		.val.s = "nop",
		.proc = b6b_interp_proc_nop
	},
	{
		.name = "yield",
		.type = B6B_TYPE_STR,
		.val.s = "yield",
		.proc = b6b_interp_proc_yield
	},
	{
		.name = "return",
		.type = B6B_TYPE_STR,
		.val.s = "return",
		.proc = b6b_interp_proc_return
	},
	{
		.name = "continue",
		.type = B6B_TYPE_STR,
		.val.s = "continue",
		.proc = b6b_interp_proc_continue
	},
	{
		.name = "break",
		.type = B6B_TYPE_STR,
		.val.s = "break",
		.proc = b6b_interp_proc_break
	},
	{
		.name = "throw",
		.type = B6B_TYPE_STR,
		.val.s = "throw",
		.proc = b6b_interp_proc_throw
	},
	{
		.name = "exit",
		.type = B6B_TYPE_STR,
		.val.s = "exit",
		.proc = b6b_interp_proc_exit
	},
	{
		.name = "local",
		.type = B6B_TYPE_STR,
		.val.s = "local",
		.proc = b6b_interp_proc_local
	},
	{
		.name = "global",
		.type = B6B_TYPE_STR,
		.val.s = "global",
		.proc = b6b_interp_proc_global
	},
	{
		.name = "export",
		.type = B6B_TYPE_STR,
		.val.s = "export",
		.proc = b6b_interp_proc_export
	},
	{
		.name = "eval",
		.type = B6B_TYPE_STR,
		.val.s = "eval",
		.proc = b6b_interp_proc_eval
	},
	{
		.name = "repr",
		.type = B6B_TYPE_STR,
		.val.s = "repr",
		.proc = b6b_interp_proc_repr
	},
	{
		.name = "call",
		.type = B6B_TYPE_STR,
		.val.s = "call",
		.proc = b6b_interp_proc_call
	},
	{
		.name = "b6b",
		.type = B6B_TYPE_STR,
		.val.s = "b6b",
		.proc = b6b_interp_proc_b6b
	},
	{
		.name = "spawn",
		.type = B6B_TYPE_STR,
		.val.s = "spawn",
		.proc = b6b_interp_proc_spawn
	}
};
__b6b_ext(b6b_interp);
