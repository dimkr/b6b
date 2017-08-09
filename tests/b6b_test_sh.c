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
#include <string.h>
#include <elf.h>

#include <b6b.h>

int main()
{
	struct b6b_interp interp;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh {echo -n 6}] {{$local p [$poll]} {$p add [$out fd] $POLLIN} {$p wait 2 1000} {$out read}}}", 118) == B6B_OK);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh {echo -n 6 1>&2}] {{$local p [$poll]} {$p add [$err fd] $POLLIN} {$p wait 2 1000} {$err read}}}", 123) == B6B_OK);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh {echo -n 6 2>/dev/null}] {{$local p [$poll]} {$p add [$out fd] $POLLIN} {$p wait 2 1000} {$out read}}}", 130) == B6B_OK);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh base64] {{$local p [$poll]} {$p add [$in fd] $POLLIN} {$p wait 2 1000} {$in write abcd} {$in close} {$p add [$out fd] $POLLOUT} {$p wait 2 1000} {$rtrim [$out read]}}}", 195) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "YWJjZA==") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh base64] {{$local p [$poll]} {$p add [$in fd] $POLLIN} {$p wait 2 1000} {$in writeln abcd} {$in close} {$p add [$out fd] $POLLOUT} {$p wait 2 1000} {$rtrim [$out read]}}}", 197) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "YWJjZAo=") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh {cat /bin/sh}] {{$local p [$poll]} {$p add [$out fd] $POLLOUT} {$p wait 2 1000} {$out read 4}}}", 123) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == SELFMAG);
	assert(memcmp(interp.fg->_->s, ELFMAG, SELFMAG) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp, "{$map {pid in out err} [$sh {cat < /dev/zero}] {{$local p [$poll]} {$p add [$in fd] $POLLIN} {$p wait 2 5} {$p add [$out fd] $POLLOUT} {$p wait 2 1000} {$out read 17}}}", 168) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 17);
	assert(memcmp(interp.fg->_->s, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 17) == 0);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
