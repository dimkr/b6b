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

#include <ucontext.h>

#define B6B_NTHREADS 8

enum b6b_thread_flags {
	B6B_THREAD_BG      = 1,
	B6B_THREAD_FG      = 1 << 1,
	B6B_THREAD_DONE    = 1 << 2,
	B6B_THREAD_NATIVE  = 1 << 3
};

struct b6b_thread {
	ucontext_t ucp;
	void *stack;
	struct b6b_frame *curr;
	unsigned int depth;
	struct b6b_obj *fn;
	struct b6b_obj *_;
	uint8_t flags;
};

int b6b_thread_init(struct b6b_thread *t,
                    struct b6b_thread *next,
                    struct b6b_obj *fn,
                    struct b6b_frame *global,
                    struct b6b_obj *null,
                    void (*routine)(int, int, int, int),
                    void *priv,
                    const size_t stksiz);

void b6b_thread_reset(struct b6b_thread *t);
void b6b_thread_destroy(struct b6b_thread *t);

void b6b_thread_swap(struct b6b_thread *bg, struct b6b_thread *fg);
