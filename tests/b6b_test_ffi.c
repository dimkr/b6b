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

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	struct b6b_obj *b, *i;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.buf abcd}", 15) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.buf abc\0d}", 16) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 5);
	assert(memcmp(interp.fg->_->s, "abc\0d", 5) == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.memcpy [[$ffi.buf abcd] address] 3}",
	                     41) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(strcmp(interp.fg->_->s, "abc") == 0);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.pack {}}", 14) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.pack b}", 13) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.pack bb 100}", 18) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.pack bi 122 1633837924}", 29) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen >= sizeof(unsigned char) + sizeof(int));
	assert(interp.fg->_->slen % 2 == 0);
	assert((unsigned char)interp.fg->_->s[0] == 122);
	assert(*(int *)&interp.fg->_->s[interp.fg->_->slen - sizeof(int)] == 1633837924);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.pack .bi 122 1633837924}", 30) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == sizeof(unsigned char) + sizeof(int));
	assert((unsigned char)interp.fg->_->s[0] == 122);
	assert(*(int *)&interp.fg->_->s[interp.fg->_->slen - sizeof(int)] == 1633837924);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.unpack bi [$ffi.pack bi 122 1633837924]}",
	                     46) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_parse(interp.fg->_, "ii", &b, &i) == 2);
	assert(b->i == 122);
	assert(i->i == 1633837924);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.unpack .bi [$ffi.pack .bi 122 1633837924]}",
	                     48) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_parse(interp.fg->_, "ii", &b, &i) == 2);
	assert(b->i == 122);
	assert(i->i == 1633837924);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.unpack bi [$ffi.pack .bi 122 1633837924]}",
	                     47) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.dlopen libz.so.1}", 23) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.dlopen {}}", 16) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.dlopen abcd}", 18) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$ffi.dlopen libz.so.1] dlsym crc32}",
	                     37) == B6B_OK);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{[$ffi.dlopen libz.so.1] dlsym abcd}",
	                     36) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$local a [$ffi.buf abcde]} {$ffi.call [$ffi.func [[$ffi.dlopen {}] dlsym puts] i p] [$ffi.pack p [$a address]]}",
	                     112) == B6B_OK);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 6);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$local a [$ffi.buf 00ff00ff]} {$ffi.call [$ffi.func [[$ffi.dlopen {}] dlsym strtol] l ppi] [$ffi.pack p [$a address]] [$ffi.pack p 0] [$ffi.pack i 16]}",
	                     152) == B6B_OK);
	assert(b6b_as_int(interp.fg->_));
	assert(interp.fg->_->i == 16711935);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
