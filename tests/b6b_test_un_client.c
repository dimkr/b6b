/*
 * This file is part of b6b.
 *
 * Copyright 2018 Dima Krasner
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifndef __SANITIZE_ADDRESS__
#	include <sys/time.h>
#	include <sys/resource.h>
#endif

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
#ifndef __SANITIZE_ADDRESS__
	struct rlimit rlim;
#endif

	assert((unlink("/tmp/x") == 0) || (errno == ENOENT));
	assert((unlink("/tmp/abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabc") == 0) || (errno == ENOENT));

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.client}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.client stream}", 19) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.client stream /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$local s [$un.server stream /tmp/x]}",
	                     37) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                    "{$local c [$un.client stream /tmp/x]}",
	                    37) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$c peer}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == sizeof("/tmp/x") - 1);
	assert(strcmp(interp.fg->_->s, "/tmp/x") == 0);
	assert(b6b_call_copy(&interp,
	                     "{$local a [$list.index [$s accept] 0 ]}",
	                     39) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$a write abcd}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$c read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(b6b_call_copy(&interp, "{$c write efgh}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "efgh") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$local s [$un.server dgram /tmp/x]}",
	                     36) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                    "{$local c [$un.client dgram /tmp/x]}",
	                    36) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$c write abcd}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(b6b_call_copy(&interp, "{$s read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$un.client dgram /tmp/x}", 25) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server dgram /tmp/x}", 25) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$un.client stream /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$un.client streaf /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);

	/* the address length is exactly 108, UNIX_PATH_MAX */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$un.server stream /tmp/abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabc}",
	                     128) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$un.client stream /tmp/abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabc}",
	                     128) == B6B_OK);
	b6b_interp_destroy(&interp);

#ifndef __SANITIZE_ADDRESS__
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(getrlimit(RLIMIT_NOFILE, &rlim) == 0);
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	rlim.rlim_cur = 1;
	assert(setrlimit(RLIMIT_NOFILE, &rlim) == 0);
	assert(b6b_call_copy(&interp, "{$un.client stream /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);
#endif

	return EXIT_SUCCESS;
}
