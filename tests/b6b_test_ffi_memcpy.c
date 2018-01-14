/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2018 Dima Krasner
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <b6b.h>

int main()
{
	static char stmt[128];
	struct b6b_interp interp;
	const char *s;
	size_t len;
	int out;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.memcpy}", 13) == B6B_ERR);
	b6b_interp_destroy(&interp);

	s = gai_strerror(EAI_SERVICE);
	assert(s);

	len = strlen(s);
	assert(len > 0);

	/* reading a string should succeed */
	out = sprintf(stmt, "{$ffi.memcpy %"PRIuPTR" %zu}", (uintptr_t)s, len);
	assert(out > 0);
	assert(out < sizeof(stmt));

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, stmt, (size_t)out) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == len);
	assert(strcmp(interp.fg->_->s, s) == 0);
	b6b_interp_destroy(&interp);

	/* reading a substring should succeed */
	out = sprintf(stmt, "{$ffi.memcpy %"PRIuPTR" 5}", (uintptr_t)s);
	assert(out > 0);
	assert(out < sizeof(stmt));

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, stmt, (size_t)out) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 5);
	assert(memcmp(interp.fg->_->s, s, 5) == 0);
	b6b_interp_destroy(&interp);

	/* reading 0 bytes should fail */
	out = sprintf(stmt, "{$ffi.memcpy %"PRIuPTR" 0}", (uintptr_t)s);
	assert(out > 0);
	assert(out < sizeof(stmt));

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, stmt, (size_t)out) == B6B_ERR);
	b6b_interp_destroy(&interp);

	/* attempts to read from NULL should be detected */
	out = sprintf(stmt, "{$ffi.memcpy %"PRIuPTR" 5}", (uintptr_t)NULL);
	assert(out > 0);
	assert(out < sizeof(stmt));

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, stmt, (size_t)out) == B6B_ERR);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
