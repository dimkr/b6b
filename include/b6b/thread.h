/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2020 Dima Krasner
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

#ifdef B6B_HAVE_THREADS

#	include <ucontext.h>
#	include <setjmp.h>

#	include <sys/queue.h>

enum b6b_thread_flags {
	B6B_THREAD_BG      = 1,
	B6B_THREAD_FG      = 1 << 1,
	B6B_THREAD_DONE    = 1 << 2,
	B6B_THREAD_BLOCKED = 1 << 3,
	B6B_THREAD_OS      = 1 << 4
};

enum b6b_context_type {
	B6B_CONTEXT_TYPE_JMP,
	B6B_CONTEXT_TYPE_SWITCH
};

TAILQ_HEAD(b6b_threads, b6b_thread);
struct b6b_thread {
	union {
		jmp_buf env;
		ucontext_t ucp;
	};
	enum b6b_context_type type;
	void *stack;
	struct b6b_frame *curr;
	struct b6b_obj *fn;
	struct b6b_obj *_;
	TAILQ_ENTRY(b6b_thread) ents;
#	ifdef B6B_HAVE_VALGRIND
	int sid;
#	endif
	unsigned int depth;
	uint8_t flags;
};

#	define b6b_thread_init(h) TAILQ_INIT(h)
#	define b6b_thread_first(h) TAILQ_FIRST(h)
#	define b6b_thread_next(t) TAILQ_NEXT(t, ents)
#	define b6b_thread_foreach(h, t) TAILQ_FOREACH(t, h, ents)
#	define b6b_thread_foreach_safe(h, t, tt) TAILQ_FOREACH_SAFE(t, h, ents, tt)

struct b6b_thread *b6b_thread_new(struct b6b_threads *threads,
                                  struct b6b_obj *fn,
                                  struct b6b_frame *global,
                                  struct b6b_obj *null,
                                  void (*routine)(int, int, int, int),
                                  void *priv,
                                  const size_t stksiz);

static inline void b6b_thread_push(struct b6b_threads *threads,
                                   struct b6b_thread *t,
                                   struct b6b_thread *fg)
{
	/* if this isn't the first thread, we want to let it run immediately */
	if (fg)
		TAILQ_INSERT_AFTER(threads, fg, t, ents);
	else
		TAILQ_INSERT_TAIL(threads, t, ents);
}

void b6b_thread_pop(struct b6b_threads *ts, struct b6b_thread *t);

void b6b_thread_swap(struct b6b_thread *bg, struct b6b_thread *fg);

#	define b6b_thread_block(t) \
	(t)->flags |= B6B_THREAD_BLOCKED
#	define b6b_thread_unblock(t) \
	(t)->flags &= ~B6B_THREAD_BLOCKED
#	define b6b_thread_blocked(t) \
	((t)->flags & B6B_THREAD_BLOCKED)

#else

struct b6b_thread {
	struct b6b_obj *_;
	struct b6b_frame *curr;
	unsigned int depth;
};

#endif

struct b6b_thread *b6b_thread_self(struct b6b_frame *global,
                                   struct b6b_obj *null);
void b6b_thread_destroy(struct b6b_thread *t);
