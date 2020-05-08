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

#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <b6b.h>

__attribute__((noreturn))
static void *b6b_offload_thread_routine(void *arg)
{
	struct b6b_offload_thread *t = (struct b6b_offload_thread *)arg;
	int sig;

	/* block all signals */
	if (pthread_sigmask(SIG_BLOCK, &t->mask, NULL) != 0)
		pthread_exit(NULL);

	/* notify the main thread that initialization is complete */
	atomic_store(&t->state, B6B_OFFLOAD_UP);
	if (pthread_kill(t->main, t->sig) != 0)
		pthread_exit(NULL);

	/* block until t->sig; we wait only for this signal so we don't unqueue
	 * signals handled by other threads */
	while ((sigwait(&t->wmask, &sig) == 0) &&
	       (atomic_load(&t->state) == B6B_OFFLOAD_RUNNING)) {
		t->fn(t->arg);

		/* signal b6b_offload_done() - we need this state to prevent a thread
		 * from calling b6b_offload_start() (hence, changing the state to
		 * B6B_OFFLOAD_RUNNING) when the previous user of the offload thread
		 * calls b6b_yield() while waiting for completion */
		atomic_store(&t->state, B6B_OFFLOAD_DONE);

		/* in addition, notify the main thread using a signal: in the special
		 * case of a single b6b thread we also need an alternative, blocking
		 * mechanism for waiting until b6b_offload_done(); without this, the
		 * only way to wait until b6b_offload_done() is a CPU-heavy loop of
		 * b6b_yield() which doesn't do anything because there's no other
		 * thread to switch to */
		if (pthread_kill(t->main, t->sig) != 0)
			pthread_exit(NULL);
	}

	pthread_exit(NULL);
}

int b6b_offload_thread_start(struct b6b_offload_thread *t, const int id)
{
	int sig;

	if ((sigfillset(&t->mask) < 0) || (sigemptyset(&t->wmask) < 0))
		return 0;

	/* pick a high realtime signal and add it to both masks */
	t->sig = SIGRTMAX - id - 1;
	assert(t->sig >= SIGRTMIN);
	if ((t->sig < SIGRTMIN) ||
	    (sigaddset(&t->mask, t->sig) < 0) ||
	    (sigaddset(&t->wmask, t->sig) < 0))
		return 0;

	/* block only the realtime signal; we do not restore the signal mask because
	 * this is racy in multi-threaded processes that may alter their signal mask
	 * after we blocked t->sig */
	if (pthread_sigmask(SIG_BLOCK, &t->wmask, NULL) != 0)
		return 0;

	t->main = pthread_self();
	if (pthread_create(&t->tid, NULL, b6b_offload_thread_routine, t) != 0)
		return 0;

	/* wait for the offload thread to send the realtime signal once
	 * initialization is complete */
	do {
		if ((sigwait(&t->wmask, &sig) != 0) || (sig != t->sig)) {
			pthread_cancel(t->tid);
			pthread_join(t->tid, NULL);
			return 0;
		}
	} while (atomic_load(&t->state) != B6B_OFFLOAD_UP);

	/* signal b6b_offload_ready() */
	atomic_store(&t->state, B6B_OFFLOAD_IDLE);

	return 1;
}

void b6b_offload_thread_stop(struct b6b_offload_thread *t)
{
	if (atomic_load(&t->state) != B6B_OFFLOAD_INIT) {
		pthread_cancel(t->tid);
		pthread_join(t->tid, NULL);
	}
}

int b6b_offload_start(struct b6b_offload_thread *t,
                      void (*fn)(void *),
                      void *arg)
{
	/* falsify b6b_offload_ready() */
	atomic_store(&t->state, B6B_OFFLOAD_RUNNING);
	t->fn = fn;
	t->arg = arg;

	/* wake up the offload thread */
	if (pthread_kill(t->tid, t->sig) != 0)
		return 0;

	return 1;
}

int b6b_offload_finish(struct b6b_offload_thread *t)
{
	int sig;

	/* sigwait() should return almost instantly if b6b_offload_done(); we call
	 * it to unqueue the signal */
	if ((sigwait(&t->wmask, &sig) != 0) || (sig != t->sig))
		return 0;

	atomic_store(&(t)->state, B6B_OFFLOAD_IDLE);
	return 1;
}
