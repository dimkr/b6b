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

#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>
#ifdef B6B_HAVE_VALGRIND
#	include <valgrind/helgrind.h>
#endif

/* simple state machine: b6b_offload_thread.state is B6B_OFFLOAD_IDLE when no
 * offload is running, transitions to B6B_OFFLOAD_RUNNING before preparations
 * for execution and to B6B_OFFLOAD_DONE when the offload returns */
enum b6b_offload_state {
	B6B_OFFLOAD_INIT,
	B6B_OFFLOAD_UP,
	B6B_OFFLOAD_IDLE,
	B6B_OFFLOAD_RUNNING,
	B6B_OFFLOAD_DONE
};

struct b6b_offload_thread {
	sigset_t mask;
	sigset_t wmask;
	pthread_t tid;
	pthread_t main;
	void (*fn)(void *);
	void *arg;
	atomic_int state;
	int sig;
};

static inline void b6b_offload_thread_init(struct b6b_offload_thread *t)
{
	atomic_store(&t->state, B6B_OFFLOAD_INIT);

#ifdef B6B_HAVE_VALGRIND
	/* all operations on t->state are atomic */
	VALGRIND_HG_DISABLE_CHECKING(&t->state, sizeof(t->state));

	/* stores are guarded by t->state and loads happen only after delivery of
	 * t->sig */
	VALGRIND_HG_DISABLE_CHECKING(&t->fn, sizeof(t->fn));
	VALGRIND_HG_DISABLE_CHECKING(&t->arg, sizeof(t->arg));
#endif
}

int b6b_offload_thread_start(struct b6b_offload_thread *t);
void b6b_offload_thread_stop(struct b6b_offload_thread *t);

/* these two macros are used to poll the state machine */
#define b6b_offload_ready(t) (atomic_load(&(t)->state) == B6B_OFFLOAD_IDLE)
#define b6b_offload_done(t) (atomic_load(&(t)->state) == B6B_OFFLOAD_DONE)

int b6b_offload_start(struct b6b_offload_thread *t,
                      void (*fn)(void *),
                      void *arg);

int b6b_offload_finish(struct b6b_offload_thread *t);
