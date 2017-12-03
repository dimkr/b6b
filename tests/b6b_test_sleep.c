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

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include <b6b.h>

static void on_alrm(int sig)
{
}

int main()
{
	struct b6b_interp interp;
	struct timespec ts1, ts2;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$sleep}", 8) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$sleep -1}", 11) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(clock_gettime(CLOCK_MONOTONIC, &ts1) == 0);
	assert(b6b_call_copy(&interp, "{$sleep 0}", 10) == B6B_OK);
	assert(clock_gettime(CLOCK_MONOTONIC, &ts2) == 0);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) < 0.1);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(clock_gettime(CLOCK_MONOTONIC, &ts1) == 0);
	assert(b6b_call_copy(&interp, "{$sleep 1}", 10) == B6B_OK);
	assert(clock_gettime(CLOCK_MONOTONIC, &ts2) == 0);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) > 0.9);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) < 1.1);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(clock_gettime(CLOCK_MONOTONIC, &ts1) == 0);
	assert(b6b_call_copy(&interp, "{$sleep 0.5}", 12) == B6B_OK);
	assert(clock_gettime(CLOCK_MONOTONIC, &ts2) == 0);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) > 0.4);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) < 0.6);
	b6b_interp_destroy(&interp);

	/* nanosleep() should be interrupted with a signal, then called again with
	 * the remaining interval */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(clock_gettime(CLOCK_MONOTONIC, &ts1) == 0);
	assert(signal(SIGALRM, on_alrm) == 0);
	assert(alarm(1) == 0);
	assert(b6b_call_copy(&interp, "{$sleep 2}", 10) == B6B_OK);
	assert(clock_gettime(CLOCK_MONOTONIC, &ts2) == 0);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) > 1.9);
	assert(((double)ts2.tv_sec + (double)ts2.tv_nsec / 1000000000.0) -
	       ((double)ts1.tv_sec + (double)ts1.tv_nsec / 1000000000.0) < 2.1);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
