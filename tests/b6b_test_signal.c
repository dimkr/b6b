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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	pid_t pid = getpid();

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$signal}", 9) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$signal {}}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$signal -1}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$signal $SIGINT}", 17) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$signal $SIGINT $SIGTERM}",
	                     26) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global a [$signal $SIGINT $SIGTERM]}",
	                     38) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(!interp.fg->_->slen);
	assert(kill(pid, SIGINT) == 0);
	assert(kill(pid, SIGTERM) == 0);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == SIGINT);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == SIGTERM);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(!interp.fg->_->slen);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
