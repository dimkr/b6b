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

#include <stdlib.h>
#ifdef B6B_HAVE_THREADS
#	include <string.h>
#	include <limits.h>
#	include <signal.h>
#	ifdef B6B_HAVE_VALGRIND
#		include <valgrind/valgrind.h>
#	endif
#endif

#include <b6b.h>

void b6b_thread_destroy(struct b6b_thread *t)
{
	if (t->_)
		b6b_unref(t->_);

#ifdef B6B_HAVE_THREADS
	if (t->fn)
		b6b_unref(t->fn);
#endif

	if (t->curr)
		b6b_frame_destroy(t->curr);

#ifdef B6B_HAVE_THREADS
	if (t->stack) {
#	ifdef B6B_HAVE_VALGRIND
		VALGRIND_STACK_DEREGISTER(t->sid);
#	endif
		free(t->stack);
	}
#endif

	free(t);
}

#ifdef B6B_HAVE_THREADS

void b6b_thread_pop(struct b6b_threads *ts, struct b6b_thread *t)
{
	TAILQ_REMOVE(ts, t, ents);
	b6b_thread_destroy(t);
}

static int b6b_thread_get_context(struct b6b_thread *t)
{
	if (getcontext(&t->ucp) < 0)
		return 0;

	t->ucp.uc_stack.ss_size = t->stksiz;
	/* it's OK to assign NULL in uc_link, since we always b6b_yield() after
	 * the thread routine: the thread routine does not return */
	t->ucp.uc_link = NULL;
	t->ucp.uc_stack.ss_sp = t->stack;

	return 1;
}

static int b6b_thread_prep(struct b6b_thread *t,
                           struct b6b_obj *fn,
                           struct b6b_frame *global,
                           struct b6b_obj *null,
                           void (*routine)(int, int, int, int),
                           void *priv,
                           const size_t stksiz)
{
	memset(t, 0, sizeof(*t));

	t->curr = b6b_frame_new(global);
	if (b6b_unlikely(!t->curr))
		return 0;

	if (!t->stack) {
		t->stack = malloc(stksiz);
		if (!b6b_allocated(t->stack))
			goto bail;

#ifdef B6B_HAVE_VALGRIND
		t->sid = VALGRIND_STACK_REGISTER(t->stack, t->stack + stksiz);
#endif
	}

	t->stksiz = stksiz;

	if (!b6b_thread_get_context(t))
		goto bail;

	t->type = B6B_CONTEXT_TYPE_SWITCH;
	t->fn = b6b_ref(fn);
	t->flags = B6B_THREAD_BG;

	if (sizeof(int) < sizeof(uintptr_t))
		makecontext(&t->ucp,
		            (void (*)(void))routine,
		            4,
		            (int)((uint64_t)(uintptr_t)t >> 32),
		            (int)((uint64_t)(uintptr_t)t & UINT_MAX),
		            (int)((uint64_t)(uintptr_t)priv >> 32),
		            (int)((uint64_t)(uintptr_t)priv & UINT_MAX));
	else
		makecontext(&t->ucp,
		            (void (*)(void))routine,
		            4,
		            0,
		            (int)(uintptr_t)t,
		            0,
		            (int)(uintptr_t)priv);

	t->_ = b6b_ref(null);
	t->depth = 1;
	return 1;

bail:
	b6b_thread_destroy(t);
	return 0;
}

struct b6b_thread *b6b_thread_new(struct b6b_threads *threads,
                                  struct b6b_obj *fn,
                                  struct b6b_frame *global,
                                  struct b6b_obj *null,
                                  void (*routine)(int, int, int, int),
                                  void *priv,
                                  const size_t stksiz)
{
	struct b6b_thread *t;

	t = (struct b6b_thread *)malloc(sizeof(*t));
	if (!b6b_allocated(t))
		return NULL;

	if (!b6b_thread_prep(t,
	                     fn,
	                     global,
	                     null,
	                     routine,
	                     priv,
	                     stksiz))
		return NULL;

	return t;
}

void b6b_thread_swap(struct b6b_thread *bg, struct b6b_thread *fg)
{
	int jumped;

	fg->flags &= ~B6B_THREAD_BG;
	fg->flags |= B6B_THREAD_FG;

	bg->flags &= ~B6B_THREAD_FG;
	bg->flags |= B6B_THREAD_BG;

	jumped = (bg->type == B6B_CONTEXT_TYPE_SWITCH);

	bg->type = B6B_CONTEXT_TYPE_JMP;
	if (setjmp(bg->env) != 0)
		return;

	if (fg->type == B6B_CONTEXT_TYPE_JMP)
		longjmp(fg->env, 1);

	if (jumped)
		b6b_thread_get_context(bg);

	/* we want to use the same signal mask */
	pthread_sigmask(SIG_SETMASK, NULL, &fg->ucp.uc_sigmask);

	swapcontext(&bg->ucp, &fg->ucp);
}

#endif

struct b6b_thread *b6b_thread_self(struct b6b_frame *global,
                                   struct b6b_obj *null)
{
	struct b6b_thread *t;

	t = (struct b6b_thread *)malloc(sizeof(*t));
	if (!b6b_allocated(t))
		return NULL;

	t->curr = b6b_frame_new(global);
	if (b6b_unlikely(!t->curr)) {
		free(t);
		return NULL;
	}

#ifdef B6B_HAVE_THREADS
	t->type = B6B_CONTEXT_TYPE_SWITCH;
	t->stack = NULL;
	t->fn = NULL;
	t->flags = B6B_THREAD_FG;
#endif
	t->_ = b6b_ref(null);
	t->depth = 1;
	return t;
}
