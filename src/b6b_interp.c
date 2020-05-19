/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2018, 2020 Dima Krasner
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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>

#include <b6b.h>

#ifdef B6B_HAVE_THREADS

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

#endif

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
	int j;

	interp->fg = NULL;
#ifdef B6B_HAVE_THREADS
	b6b_thread_init(&interp->threads);
#	ifdef B6B_HAVE_OFFLOAD_THREAD
	interp->noffths = sizeof(interp->offths) / sizeof(interp->offths[0]);
	if (b6b_unlikely(opts & B6B_OPT_NO_POOL))
		interp->noffths = 1;

	for (j = 0; j < interp->noffths; ++j)
		b6b_offload_thread_init(&interp->offths[j]);
#	endif

	/* we allocate a small stack (just two pages) */
	interp->stksiz = sysconf(_SC_PAGESIZE);
	if (interp->stksiz <= 0)
		goto bail;

#	ifdef B6B_HAVE_OFFLOAD_THREAD
	for (j = 0; j < interp->noffths; ++j) {
		if (!b6b_offload_thread_start(&interp->offths[j], j))
			goto bail;
	}
#	endif

	interp->stksiz *= 2;
#endif

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

#ifdef B6B_HAVE_THREADS
	interp->qstep = 0;
	interp->exit = 0;
#endif

	interp->fg = b6b_thread_self(interp->global, interp->null);
	if (!interp->fg)
		goto bail;
	interp->self = interp->fg;
#ifdef B6B_HAVE_THREADS
	b6b_thread_push(&interp->threads, interp->fg, NULL);
#endif

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
#ifdef B6B_HAVE_THREADS
	struct b6b_thread *t;
	int i;

	/* wait until all threads except the main thread are inactive */
	interp->exit = 1;
	do {} while (b6b_yield(interp));

	/* destroy the main thread */
	t = b6b_thread_first(&interp->threads);
	if (t)
		b6b_thread_pop(&interp->threads, t);

#	ifdef B6B_HAVE_OFFLOAD_THREAD
	for (i = interp->noffths - 1; i >= 0; --i)
		b6b_offload_thread_stop(&interp->offths[i]);
#	endif

#else
	if (interp->fg)
		b6b_thread_destroy(interp->fg);
#endif
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
	struct b6b_obj name, *o, *stmt;
	enum b6b_res res;

	if (exp->slen) {
		switch (exp->s[0]) {
			case '$':
				/* by using a statically-allocated object that points to the
				 * same buffer as exp, we avoid the huge overhead of two
				 * malloc() calls (one for the object and another for the
				 * string); the performance gain is huge */
				b6b_str_init(&name, &exp->s[1], exp->slen - 1);

				o = b6b_get(interp, &name);
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

				stmt = b6b_list_from(&exp->s[1], exp->slen - 2);
				if (b6b_unlikely(!stmt))
					return B6B_ERR;

				res = b6b_stmt_call(interp, stmt);
				b6b_unref(stmt);
				return res;
		}
	}

	return b6b_return(interp, b6b_ref(exp));
}

#ifdef B6B_HAVE_THREADS

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
	/* we must restore the main thread's signal mask */
	if ((bg != interp->self) && b6b_thread_save(bg))
		return 1;
	interp->fg = t;
	interp->qstep = 0;
	b6b_thread_swap(bg, interp->fg);
	b6b_wait(interp);
	return 1;
}

#endif

#ifdef B6B_HAVE_OFFLOAD_THREAD

static struct b6b_offload_thread *b6b_pick_thread(struct b6b_interp *interp)
{
	unsigned int i;

	for (i = 0; i < interp->noffths; ++i) {
		if (b6b_offload_ready(&interp->offths[i]))
			return &interp->offths[i];
	}

	return NULL;
}

int b6b_offload(struct b6b_interp *interp,
                void (*fn)(void *),
                void *arg)
{
	struct b6b_thread *t;
	int swap, pending = 0;
	struct b6b_offload_thread *offth = NULL;

	if (!b6b_threaded(interp))
		fn(arg);
	else {
		b6b_thread_block(interp->fg);

		do {
			offth = b6b_pick_thread(interp);
			if (offth)
				break;

			/* if the offload thread is busy but no other thread is running,
			 * this is clearly a serious bug */
			if (!b6b_yield(interp)) {
				b6b_thread_unblock(interp->fg);
				return 0;
			}
		} while (1);

		if (!b6b_offload_start(offth, fn, arg)) {
			b6b_thread_unblock(interp->fg);
			return 0;
		}

		do {
			/* if this is the only remaining active thread, we cannot poll for
			 * b6b_offload_done() since b6b_yield() doesn't do anything; in this
			 * case, we let b6b_offload_finish() block */
			if (!b6b_threaded(interp))
				break;

			/* check whether there is at least one thread that waits for us to
			 * complete the blocking operation and at least one thread which
			 * doesn't */
			pending = swap = 0;
			b6b_thread_foreach(&interp->threads, t) {
				if (t != interp->fg) {
					if (b6b_thread_blocked(t)) {
						pending = 1;
						if (swap)
							break;
					}
					else {
						swap = 1;
						if (pending)
							break;
					}
				}
			}

			/* if all other threads are blocked by us, we want to stop this
			 * polling loop since all they do is repeatedly calling
			 * b6b_yield() */
			if (!swap)
				break;

			/* if all other threads exited during the previous iterations of
			 * this loop, we can (and should) block */
			if (!b6b_yield(interp))
				break;
		} while (!b6b_offload_done(offth));

		b6b_thread_unblock(interp->fg);

		if (!b6b_offload_finish(offth))
			return 0;

		/* if there's at least one thread which waits for us to complete the
		 * blocking operation, we must give it a chance to run - otherwise, if
		 * the next statement of the current thread also calls
		 * b6b_offload_start(), that thread will spend more time in its
		 * b6b_offload_ready() loop */
		if (pending)
			b6b_yield(interp);
	}

	return 1;
}

#endif

static enum b6b_res b6b_on_res(struct b6b_interp *interp,
                               const enum b6b_res res)
{
	if (res == B6B_YIELD) {
		b6b_yield(interp);
		return B6B_OK;
	}

#ifdef B6B_HAVE_THREADS
	++interp->qstep;
#endif

	/* update _ of the calling frame */
	if (b6b_unlikely(!b6b_local(interp, interp->_, interp->fg->_)))
		return B6B_ERR;

#ifdef B6B_HAVE_THREADS
	if (res == B6B_EXIT) {
		/* signal all threads to stop */
		interp->exit = 1;
		return res;
	}

	/* switch to another thread after each batch of B6B_QUANT_LEN statements */
	if (interp->qstep == B6B_QUANT_LEN)
		b6b_yield(interp);
#endif

	return res;
}

static enum b6b_res b6b_stmt_call(struct b6b_interp *interp,
                                  struct b6b_obj *stmt)
{
	struct b6b_litem *li;
	struct b6b_frame *f;
	const char *p;
	enum b6b_res res = B6B_ERR;

#ifdef B6B_HAVE_THREADS
	/* if the interpreter is exiting, issue B6B_EXIT in the current thread;
	 * b6b_join() takes care of sweeping through all threads */
	if (interp->exit)
		return B6B_EXIT;
#endif

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

	if (interp->opts & B6B_OPT_TRACE) {
		if (!b6b_as_str(f->args))
			goto pop;

		if (fwrite("+ ", 2, 1, stderr) != 1)
			goto pop;

		p = strchr(f->args->s, '\n');
		if (p) {
			if ((fwrite(f->args->s, p - f->args->s, 1, stderr) != 1) ||
			    (fwrite(" ...\n", 5, 1, stderr) != 1))
				goto pop;
		}
		else if ((fwrite(f->args->s, f->args->slen, 1, stderr) != 1) ||
		         (fputc('\n', stderr) != '\n'))
			goto pop;
	}

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

#ifdef B6B_HAVE_THREADS

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

#endif

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
	if (!b6b_allocated(s)) {
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
	if (!b6b_allocated(b6b))
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

static const struct b6b_ext_obj b6b_interp[] = {
	{
		.name = "b6b",
		.type = B6B_TYPE_STR,
		.val.s = "b6b",
		.proc = b6b_interp_proc_b6b
	}
};
__b6b_ext(b6b_interp);
