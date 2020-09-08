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

static void b6b_offload_cond_init(struct b6b_offload_cond *cond)
{
	pthread_mutex_init(&cond->lock, NULL);
	pthread_cond_init(&cond->cond, NULL);
}

static void b6b_offload_cond_destroy(struct b6b_offload_cond *cond)
{
	pthread_cond_destroy(&cond->cond);
	pthread_mutex_destroy(&cond->lock);
}

static void b6b_offload_cond_cleanup(void *lock)
{
	pthread_mutex_unlock((pthread_mutex_t *)lock);
}

static void b6b_offload_cond_wait(struct b6b_offload_thread *t,
                                  const enum b6b_offload_state next)
{
	pthread_mutex_lock(&t->cond.lock);
	pthread_cleanup_push(b6b_offload_cond_cleanup, &t->cond.lock);
	while (atomic_load(&t->state) != next)
		pthread_cond_wait(&t->cond.cond, &t->cond.lock);
	pthread_cleanup_pop(1);
}

static void b6b_offload_cond_notify(struct b6b_offload_thread *t,
                                    const enum b6b_offload_state next)
{
	pthread_mutex_lock(&t->cond.lock);
	atomic_store(&t->state, next);
	pthread_cond_signal(&t->cond.cond);
	pthread_mutex_unlock(&t->cond.lock);
}

__attribute__((noreturn))
static void *b6b_offload_thread_routine(void *arg)
{
	struct b6b_offload_thread *t = (struct b6b_offload_thread *)arg;

	/* block all signals */
	if (pthread_sigmask(SIG_BLOCK, &t->mask, NULL) != 0)
		pthread_exit(NULL);

	/* notify the main thread that initialization is complete */
	b6b_offload_cond_notify(t, B6B_OFFLOAD_UP);

	while (1) {
		b6b_offload_cond_wait(t, B6B_OFFLOAD_RUNNING);

		t->fn(t->arg);

		/* signal b6b_offload_done() - we need this state to prevent a thread
		 * from calling b6b_offload_start() (hence, changing the state to
		 * B6B_OFFLOAD_RUNNING) when the previous user of the offload thread
		 * calls b6b_yield() while waiting for completion */
		b6b_offload_cond_notify(t, B6B_OFFLOAD_DONE);
	}

	pthread_exit(NULL);
}

int b6b_offload_thread_start(struct b6b_offload_thread *t, const int id)
{
	if (sigfillset(&t->mask) < 0)
		return 0;

	b6b_offload_cond_init(&t->cond);

	t->main = pthread_self();
	if (pthread_create(&t->tid, NULL, b6b_offload_thread_routine, t) != 0) {
		b6b_offload_cond_destroy(&t->cond);
		return 0;
	}

	b6b_offload_cond_wait(t, B6B_OFFLOAD_UP);

	/* signal b6b_offload_ready() */
	atomic_store(&t->state, B6B_OFFLOAD_IDLE);

	return 1;
}

void b6b_offload_thread_stop(struct b6b_offload_thread *t)
{
	if (atomic_load(&t->state) != B6B_OFFLOAD_INIT) {
		pthread_cancel(t->tid);
		pthread_join(t->tid, NULL);

		b6b_offload_cond_destroy(&t->cond);
	}
}

int b6b_offload_start(struct b6b_offload_thread *t,
                      void (*fn)(void *),
                      void *arg)
{
	t->fn = fn;
	t->arg = arg;

	b6b_offload_cond_notify(t, B6B_OFFLOAD_RUNNING);

	return 1;
}

int b6b_offload_finish(struct b6b_offload_thread *t)
{
	b6b_offload_cond_wait(t, B6B_OFFLOAD_DONE);

	atomic_store(&(t)->state, B6B_OFFLOAD_IDLE);
	return 1;
}
