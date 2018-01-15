/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2018 Dima Krasner
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

#include <inttypes.h>

#define B6B_QUANT_LEN 16

enum b6b_interp_opts {
	B6B_OPT_CMD   = 1,
	B6B_OPT_RAISE = 1 << 1,
	B6B_OPT_NBF   = 1 << 2,
	B6B_OPT_TRACE = 1 << 3
};

struct b6b_interp {
#ifdef B6B_HAVE_THREADS
	struct b6b_threads threads;
#	ifdef B6B_HAVE_OFFLOAD_THREAD
	struct b6b_offload_thread offth;
#	endif
#endif
	struct b6b_thread *fg;
	struct b6b_frame *global;
	struct b6b_obj *null;
	struct b6b_obj *zero;
	struct b6b_obj *one;
	struct b6b_obj *dot;
	struct b6b_obj *at;
	struct b6b_obj *_;
#ifdef B6B_HAVE_THREADS
	long stksiz;
	int exit;
#endif
	unsigned int seed;
#ifdef B6B_HAVE_THREADS
	uint8_t qstep;
#endif
	uint8_t opts;
};

#ifdef B6B_HAVE_THREADS

static inline int b6b_threaded(struct b6b_interp *interp)
{
	const struct b6b_thread *t = b6b_thread_first(&interp->threads);
	return (t && b6b_thread_next(t)) ? 1 : 0;
}

#else

static inline int b6b_threaded(struct b6b_interp *interp)
{
	return 0;
}

#endif

int b6b_interp_new(struct b6b_interp *interp,
                   struct b6b_obj *args,
                   const uint8_t opts);
int b6b_interp_new_argv(struct b6b_interp *interp,
                        const int argc,
                        const char *argv[],
                        const uint8_t opts);
void b6b_interp_destroy(struct b6b_interp *interp);

static inline int b6b_local(struct b6b_interp *interp,
                            struct b6b_obj *k,
                            struct b6b_obj *v)
{
	return b6b_dict_set(interp->fg->curr->locals, k, v);
}

static inline int b6b_global(struct b6b_interp *interp,
                             struct b6b_obj *k,
                             struct b6b_obj *v)
{
	return b6b_dict_set(interp->global->locals, k, v);
}

struct b6b_obj *b6b_get(struct b6b_interp *interp, struct b6b_obj *name);

enum b6b_res b6b_eval(struct b6b_interp *interp, struct b6b_obj *exp);
enum b6b_res b6b_call(struct b6b_interp *interp, struct b6b_obj *stmts);
enum b6b_res b6b_call_copy(struct b6b_interp *interp,
                           const char *s,
                           const size_t len);
#ifdef B6B_HAVE_THREADS
int b6b_start(struct b6b_interp *interp, struct b6b_obj *stmts);
#endif
enum b6b_res b6b_source(struct b6b_interp *interp, const char *path);

static inline enum b6b_res b6b_return(struct b6b_interp *interp,
                                      struct b6b_obj *o)
{
	b6b_unref(interp->fg->_);
	interp->fg->_ = o;
	return B6B_OK;
}

enum b6b_res b6b_return_int(struct b6b_interp *interp, const b6b_int i);
enum b6b_res b6b_return_float(struct b6b_interp *interp, const b6b_float f);

static inline enum b6b_res b6b_return_true(struct b6b_interp *interp)
{
	return b6b_return(interp, b6b_ref(interp->one));
}

static inline enum b6b_res b6b_return_false(struct b6b_interp *interp)
{
	return b6b_return(interp, b6b_ref(interp->zero));
}

static inline enum b6b_res b6b_return_bool(struct b6b_interp *interp,
                                           const int b)
{
	if (b)
		return b6b_return_true(interp);

	return b6b_return_false(interp);
}

enum b6b_res b6b_return_str(struct b6b_interp *interp,
                            const char *s,
                            const size_t len);
enum b6b_res b6b_return_strerror(struct b6b_interp *interp, const int err);

__attribute__((format(printf, 2, 3)))
enum b6b_res b6b_return_fmt(struct b6b_interp *interp, const char *fmt, ...);

__attribute__((format(printf, 6, 7)))
unsigned int b6b_args_parse(struct b6b_interp *interp,
                            struct b6b_obj *l,
                            unsigned int min,
                            unsigned int max,
                            const char *help,
                            const char *fmt,
                            ...);

#ifdef B6B_HAVE_THREADS
int b6b_yield(struct b6b_interp *interp);
#else

static inline int b6b_yield(struct b6b_interp *interp)
{
	return 0;
}

#endif

#ifdef B6B_HAVE_OFFLOAD_THREAD

int b6b_offload(struct b6b_interp *interp,
                void (*fn)(void *),
                void *arg);

#else

static inline int b6b_offload(struct b6b_interp *interp,
                              void (*fn)(void *),
                              void *arg)
{
	fn(arg);
	return 1;
}

#endif

struct b6b_syscall_data {
	long args[6];
	int rval;
	int rerrno;
};

int b6b_syscall(struct b6b_interp *interp,
                int *ret,
                const long nr,
                ...);
