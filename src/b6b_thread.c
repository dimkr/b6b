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
#include <stdlib.h>
#include <limits.h>

#include <b6b.h>

void b6b_thread_reset(struct b6b_thread *t)
{
	if (t->_)
		b6b_unref(t->_);

	if (t->fn)
		b6b_unref(t->fn);

	if (t->curr)
		b6b_frame_destroy(t->curr);

	t->flags = 0;
}

void b6b_thread_destroy(struct b6b_thread *t)
{
	b6b_thread_reset(t);

	if (t->stack) {
		free(t->stack);
		t->stack = NULL;
	}
}

int b6b_thread_init(struct b6b_thread *t,
                    struct b6b_thread *next,
                    struct b6b_obj *fn,
                    struct b6b_frame *global,
                    struct b6b_obj *null,
                    void (*routine)(int, int, int, int),
                    void *priv,
                    const size_t stksiz)
{
	memset(t, 0, sizeof(*t));

	if (getcontext(&t->ucp) < 0)
		return 0;

	t->curr = b6b_frame_new(global);
	if (b6b_unlikely(!t->curr))
		return 0;

	if (fn) {
		if (!t->stack) {
			t->stack = malloc(stksiz);
			if (b6b_unlikely(!t->stack))
				goto bail;
		}

		t->ucp.uc_stack.ss_size = stksiz;
		t->ucp.uc_link = &next->ucp;
		t->ucp.uc_stack.ss_sp = t->stack;

		t->fn = b6b_ref(fn);
		t->flags = B6B_THREAD_BG;

		if (sizeof(int) < sizeof(uintptr_t))
			makecontext(&t->ucp,
			            (void (*)(void))routine,
			            4,
			            (int)((uint64_t)(uintptr_t)t << 32),
			            (int)((uint64_t)(uintptr_t)t & UINT_MAX),
			            (int)((uint64_t)(uintptr_t)priv << 32),
			            (int)((uint64_t)(uintptr_t)priv & UINT_MAX));
		else
			makecontext(&t->ucp,
			            (void (*)(void))routine,
			            4,
			            0,
			            (int)(uintptr_t)t,
			            0,
			            (int)(uintptr_t)priv);
	}
	else {
		t->stack = NULL;
		t->fn = NULL;
		t->flags = B6B_THREAD_FG | B6B_THREAD_NATIVE;
	}

	t->_ = b6b_ref(null);
	t->depth = 1;
	return 1;

bail:
	b6b_thread_destroy(t);
	return 0;
}

void b6b_thread_swap(struct b6b_thread *bg, struct b6b_thread *fg)
{
	fg->flags &= ~B6B_THREAD_BG;
	fg->flags |= B6B_THREAD_FG;

	bg->flags &= ~B6B_THREAD_FG;
	bg->flags |= B6B_THREAD_BG;

	swapcontext(&bg->ucp, &fg->ucp);
}
