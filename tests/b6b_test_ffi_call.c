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

#include <b6b.h>

int main()
{
	struct b6b_interp interp;

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
