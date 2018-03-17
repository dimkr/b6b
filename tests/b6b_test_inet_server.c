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
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	struct sockaddr_in dst = {.sin_family = AF_INET},
	                   src = {.sin_family = AF_INET};
	struct sockaddr_in6 dst6 = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_LOOPBACK_INIT
	}, src6 = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_LOOPBACK_INIT
	};
	char buf[4];
	int s;
	enum b6b_res res;

	src.sin_port = src6.sin6_port = htons(2923);
	src.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	dst.sin_port = dst6.sin6_port = htons(2924);
	dst.sin_addr.s_addr = src.sin_addr.s_addr;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$inet.server}", 14) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$inet.server tcp}", 18) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1}",
	                     28) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server udp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcc 127.0.0.1 2924}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 1.2.3.4 2924}",
	                     31) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp localhost 2924}",
	                     33) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp localhosf 2924}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp {} 2924}",
	                     26) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	s = socket(AF_INET, SOCK_STREAM, 0);
	assert(s >= 0);
	assert(connect(s, (const struct sockaddr *)&dst, sizeof(dst)) == 0);
	assert(b6b_call_copy(&interp,
	                     "{[$list.index [$_ accept] 0] write abcd}",
	                     40) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f == 4);
	assert(recv(s, buf, sizeof(buf), 0) == sizeof(buf));
	assert(memcmp(buf, "abcd", 4) == 0);
	assert(close(s) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$inet.server tcp 127.0.0.1 2924}",
	                     33) == B6B_OK);
	s = socket(AF_INET, SOCK_STREAM, 0);
	assert(s >= 0);
	assert(connect(s, (const struct sockaddr *)&dst, sizeof(dst)) == 0);
	assert(send(s, "abcd", 4, 0) == 4);
	assert(b6b_call_copy(&interp,
	                     "{[$list.index [$_ accept] 0] read}",
	                     34) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	assert(close(s) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global a [$inet.server udp 127.0.0.1 2924]}",
	                     45) == B6B_OK);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	assert(s >= 0);
	assert(sendto(s,
	              "abcd",
	              4,
	              0,
	              (const struct sockaddr *)&dst,
	              sizeof(dst)) == 4);
	assert(sendto(s,
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
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("127.0.0.1") - 1);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "127.0.0.1") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f > 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f < USHRT_MAX);
	assert(close(s) == 0);
	b6b_interp_destroy(&interp);


	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	res = b6b_call_copy(&interp,
	                    "{$global a [$inet.server udp ::1 2924]}",
	                    39);
	/* test is broken on Travis CI because IPv6 is not configured */
	if (res == B6B_OK) {
		s = socket(AF_INET6, SOCK_DGRAM, 0);
		assert(s >= 0);
		assert(sendto(s,
		              "abcd",
		              4,
		              0,
		              (const struct sockaddr *)&dst6,
		              sizeof(dst6)) == 4);
		assert(sendto(s,
		              "efgh",
		              4,
		              0,
		              (const struct sockaddr *)&dst6,
		              sizeof(dst6)) == 4);
		assert(b6b_call_copy(&interp, "{$a read}", 9) == B6B_OK);
		assert(b6b_as_str(interp.fg->_));
		assert(interp.fg->_->slen == 4);
		assert(strcmp(interp.fg->_->s, "abcd") == 0);
		assert(b6b_call_copy(&interp, "{$a peer}", 9) == B6B_OK);
		assert(b6b_as_list(interp.fg->_));
		assert(!b6b_list_empty(interp.fg->_));
		assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
		assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("::1") - 1);
		assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "::1") == 0);
		assert(b6b_list_next(b6b_list_first(interp.fg->_)));
		assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
		assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f > 0);
		assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f < USHRT_MAX);
		assert(close(s) == 0);
	} else
		assert(res == B6B_ERR);

	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$global a [$inet.server udp 127.0.0.1 2924]}",
	                     45) == B6B_OK);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	assert(s >= 0);
	assert(bind(s, (const struct sockaddr *)&src, sizeof(src)) == 0);
	assert(sendto(s,
	              "abcd",
	              4,
	              0,
	              (const struct sockaddr *)&dst,
	              sizeof(dst)) == 4);
	assert(sendto(s,
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
	assert(b6b_as_list(interp.fg->_));
	assert(!b6b_list_empty(interp.fg->_));
	assert(b6b_as_str(b6b_list_first(interp.fg->_)->o));
	assert(b6b_list_first(interp.fg->_)->o->slen == sizeof("127.0.0.1") - 1);
	assert(strcmp(b6b_list_first(interp.fg->_)->o->s, "127.0.0.1") == 0);
	assert(b6b_list_next(b6b_list_first(interp.fg->_)));
	assert(b6b_as_float(b6b_list_next(b6b_list_first(interp.fg->_))->o));
	assert(b6b_list_next(b6b_list_first(interp.fg->_))->o->f == 2923);
	assert(close(s) == 0);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
