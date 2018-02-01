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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#ifndef __SANITIZE_ADDRESS__
#	include <sys/time.h>
#	include <sys/resource.h>
#endif

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	struct sockaddr_un dst = {.sun_family = AF_UNIX},
	                   src = {.sun_family = AF_UNIX};
#ifndef __SANITIZE_ADDRESS__
	struct rlimit rlim;
#endif
	char buf[4];
	int s1, s2, s3;

	strcpy(src.sun_path, "/tmp/y");
	strcpy(dst.sun_path, "/tmp/x");
	assert((unlink(src.sun_path) == 0) || (errno == ENOENT));
	assert((unlink(dst.sun_path) == 0) || (errno == ENOENT));
	assert((unlink("/tmp/abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabc") == 0) || (errno == ENOENT));

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	b6b_interp_destroy(&interp);

	/* the address length is exactly 108, UNIX_PATH_MAX */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$un.server stream /tmp/abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabc}",
	                     128) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server dgram /tmp/x}", 25) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server streaf /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x 1}", 28) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$un.server stream /tmp/x 0}",
	                     28) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	s1 = socket(AF_UNIX, SOCK_STREAM, 0);
	assert(s1 >= 0);
	assert(connect(s1, (const struct sockaddr *)&dst, sizeof(dst)) == 0);
	assert(b6b_call_copy(&interp,
	                     "{[$list.index [$_ accept] 0] write abcd}",
	                     40) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(recv(s1, buf, sizeof(buf), 0) == sizeof(buf));
	assert(memcmp(buf, "abcd", 4) == 0);
	assert(close(s1) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_OK);
	s1 = socket(AF_UNIX, SOCK_STREAM, 0);
	assert(s1 >= 0);
	assert(connect(s1, (const struct sockaddr *)&dst, sizeof(dst)) == 0);
	assert(send(s1, "abcd", 4, 0) == 4);
	assert(send(s1, "efgh", 4, 0) == 4);
	assert(b6b_call_copy(&interp,
	                     "{[$list.index [$_ accept] 0] read}",
	                     34) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 8);
	assert(strcmp(interp.fg->_->s, "abcdefgh") == 0);
	assert(close(s1) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global a [$un.server dgram /tmp/x]}",
	                     37) == B6B_OK);
	s1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	assert(s1 >= 0);
	assert(sendto(s1,
	              "abcd",
	              4,
	              0,
	              (const struct sockaddr *)&dst,
	              sizeof(dst)) == 4);
	assert(sendto(s1,
	              "efgh",
	              4,
	              0,
	              (const struct sockaddr *)&dst,
	              sizeof(dst)) == 4);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(b6b_call_copy(&interp, "{$a peer}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 0);
	assert(close(s1) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global a [$un.server dgram /tmp/x]}",
	                     37) == B6B_OK);
	s1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	assert(s1 >= 0);
	assert(bind(s1, (const struct sockaddr *)&src, sizeof(src)) == 0);
	assert(sendto(s1,
	              "abcd",
	              4,
	              0,
	              (const struct sockaddr *)&dst,
	              sizeof(dst)) == 4);
	assert(sendto(s1,
	              "efgh",
	              4,
	              0,
	              (const struct sockaddr *)&dst,
	              sizeof(dst)) == 4);
	assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(b6b_call_copy(&interp, "{$a peer}", 9) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == strlen(src.sun_path));
	assert(strcmp(interp.fg->_->s, src.sun_path) == 0);
	assert(close(s1) == 0);
	assert(unlink(src.sun_path) == 0);
	b6b_interp_destroy(&interp);
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x 1}", 28) == B6B_OK);
	s1 = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	assert(s1 >= 0);
	assert(connect(s1, (const struct sockaddr *)&dst, sizeof(dst)) == 0);
	s2 = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	assert(s2 >= 0);
	assert(connect(s2, (const struct sockaddr *)&dst, sizeof(dst)) == 0);
	s3 = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	assert(s3 >= 0);
	assert(connect(s3, (const struct sockaddr *)&dst, sizeof(dst)) < 0);
	assert(errno == EAGAIN);
	assert(close(s3) == 0);
	assert(close(s2) == 0);
	assert(close(s1) == 0);
	b6b_interp_destroy(&interp);

	/* regression test: garbage values before the first read from a datagram
	 * server */
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$un.server dgram /tmp/x] peer}",
	                     32) == B6B_OK);
	assert(interp.fg->_->slen == 0);
	b6b_interp_destroy(&interp);

#ifndef __SANITIZE_ADDRESS__
	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global s [$un.server stream /tmp/x]}",
	                     38) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$global c [$un.client stream /tmp/x]}",
	                     38) == B6B_OK);

	assert(getrlimit(RLIMIT_NOFILE, &rlim) == 0);
	rlim.rlim_cur = 1;
	assert(setrlimit(RLIMIT_NOFILE, &rlim) == 0);

	assert(b6b_call_copy(&interp, "{$s accept}", 11) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_empty(interp.fg->_));
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$un.server stream /tmp/x}", 26) == B6B_ERR);
	b6b_interp_destroy(&interp);
#endif

	return EXIT_SUCCESS;
}
