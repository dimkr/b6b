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
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <signal.h>

#include <b6b.h>

__attribute__((noreturn))
static void *b6b_syscall_thread_routine(void *arg)
{
	struct b6b_syscall_thread *t = (struct b6b_syscall_thread *)arg;
	int sig;

	/* block all signals */
	if (pthread_sigmask(SIG_BLOCK, &t->mask, NULL) == 0) {
		/* signal b6b_syscall_ready() */
		atomic_store(&t->state, B6B_SYSCALL_IDLE);

		/* block until t->sig; we wait only for this signal so we don't unqueue
		 * signals handled by other threads */
		while (sigwait(&t->wmask, &sig) == 0) {
			t->ret = syscall(t->args[0],
			                 t->args[1],
			                 t->args[2],
			                 t->args[3],
			                 t->args[4],
			                 t->args[5]);
			t->rerrno = errno;

			/* signal b6b_syscall_done() */
			atomic_store(&t->state, B6B_SYSCALL_DONE);
		}
	}

	pthread_exit(NULL);
}

int b6b_syscall_thread_start(struct b6b_syscall_thread *t)
{
	if ((sigfillset(&t->mask) < 0) || (sigemptyset(&t->wmask) < 0))
		return 0;

	/* pick a high realtime signal and add it to both masks */
	t->sig = SIGRTMAX -1;
	if ((t->sig < SIGRTMIN) ||
	    (sigaddset(&t->mask, t->sig) < 0) ||
	    (sigaddset(&t->wmask, t->sig) < 0))
		return 0;

	if (pthread_create(&t->tid, NULL, b6b_syscall_thread_routine, t) != 0)
		return 0;

	/* we need this special state for b6b_syscall_thread_stop(): this state
	 * means the thread is running but not ready */
	atomic_store(&t->state, B6B_SYSCALL_UP);
	return 1;
}

void b6b_syscall_thread_stop(struct b6b_syscall_thread *t)
{
	if (atomic_load(&t->state) != B6B_SYSCALL_INIT) {
		pthread_cancel(t->tid);
		pthread_join(t->tid, NULL);
	}
}

int b6b_syscall_start(struct b6b_syscall_thread *t,
                      const long nr,
                      const long a0,
                      const long a1,
                      const long a2,
                      const long a3,
                      const long a4)
{
	/* falsify b6b_syscall_ready() */
	atomic_store(&t->state, B6B_SYSCALL_RUNNING);
	t->args[0] = nr;
	t->args[1] = a0;
	t->args[2] = a1;
	t->args[3] = a2;
	t->args[4] = a3;
	t->args[5] = a4;

	/* wake up the syscall execution thread */
	if (pthread_kill(t->tid, t->sig) != 0)
		return 0;

	return 1;
}

int b6b_syscall_finish(struct b6b_syscall_thread *t, int *ret)
{
	*ret = t->ret;
	errno = t->rerrno;

	/* signal b6b_syscall_ready() */
	atomic_store(&t->state, B6B_SYSCALL_IDLE);
	return 1;
}
