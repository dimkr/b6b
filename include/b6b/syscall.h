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
#ifdef B6B_HAVE_HELGRIND
#	include <valgrind/helgrind.h>
#endif

/* simple state machine: b6b_syscall_thread.state is B6B_SYSCALL_IDLE when no
 * syscall is running, transitions to B6B_SYSCALL_RUNNING before preparations
 * for execution and to B6B_SYSCALL_DONE when the syscall returns */
enum b6b_syscall_state {
	B6B_SYSCALL_INIT,
	B6B_SYSCALL_UP,
	B6B_SYSCALL_IDLE,
	B6B_SYSCALL_RUNNING,
	B6B_SYSCALL_DONE
};

struct b6b_syscall_thread {
	long args[6];
	sigset_t mask;
	sigset_t wmask;
	pthread_t tid;
	atomic_int state;
	int sig;
	int ret;
	int rerrno;
};

static inline void b6b_syscall_thread_init(struct b6b_syscall_thread *t)
{
	atomic_store(&t->state, B6B_SYSCALL_INIT);

#ifdef B6B_HAVE_HELGRIND
	/* all operations on t->state are atomic */
	VALGRIND_HG_DISABLE_CHECKING(&t->state, sizeof(t->state));

	/* stores to are guarded by t->state and loads happen only after delivery of
	 * t->sig */
	VALGRIND_HG_DISABLE_CHECKING(&t->args, sizeof(t->args));

	/* stores happen after delivery of t->sig and loads guarded by t->state */
	VALGRIND_HG_DISABLE_CHECKING(&t->rerrno, sizeof(t->rerrno));
	VALGRIND_HG_DISABLE_CHECKING(&t->ret, sizeof(t->ret));
#endif
}

int b6b_syscall_thread_start(struct b6b_syscall_thread *t);
void b6b_syscall_thread_stop(struct b6b_syscall_thread *t);

/* these two macros are used to poll the state machine */
#define b6b_syscall_ready(t) (atomic_load(&(t)->state) == B6B_SYSCALL_IDLE)
#define b6b_syscall_done(t) (atomic_load(&(t)->state) == B6B_SYSCALL_DONE)

int b6b_syscall_start(struct b6b_syscall_thread *t,
                      const long nr,
                      const long a0,
                      const long a1,
                      const long a2,
                      const long a3,
                      const long a4);
int b6b_syscall_finish(struct b6b_syscall_thread *t, int *ret);
