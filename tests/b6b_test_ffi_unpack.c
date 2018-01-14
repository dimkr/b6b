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

#include <b6b.h>

int main()
{
	struct b6b_interp interp;
	struct b6b_obj *os[13];

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.unpack}", 13) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.unpack {}}", 16) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.unpack bbb xy}", 20) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.unpack . xy}", 18) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.unpack bI xy}", 19) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.unpack iIlLpbBhHqQfd [$ffi.pack iIlLpbBhHqQfd 65535 65536 1048575 4294967291 1234 127 255 80 8080 4294967297 1099511627775 1.33 1.444444]}",
	                     144) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_parse(interp.fg->_, "fffffffffffff", &os[0], &os[1], &os[2], &os[3], &os[4], &os[5], &os[6], &os[7], &os[8], &os[9], &os[10], &os[11], &os[12]) == 13);
	assert(os[0]->f == 65535);
	assert(os[1]->f == 65536);
	assert(os[2]->f == 1048575);
	assert(os[3]->f == 4294967291U);
	assert(os[4]->f == 1234);
	assert(os[5]->f == 127);
	assert(os[6]->f == 255);
	assert(os[7]->f == 80);
	assert(os[8]->f == 8080);
	assert(os[9]->f == 4294967297);
	assert(os[10]->f == 1099511627775);
	assert(os[11]->f >= 1.33);
	assert(os[11]->f <= 1.331);
	assert(os[12]->f == 1.444444);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.unpack .iIlLpbBhHqQfd [$ffi.pack .iIlLpbBhHqQfd 65535 65536 1048575 4294967291 1234 127 255 80 8080 4294967297 1099511627775 1.33 1.444444]}",
	                     146) == B6B_OK);
	assert(b6b_as_list(interp.fg->_));
	assert(b6b_list_parse(interp.fg->_, "fffffffffffff", &os[0], &os[1], &os[2], &os[3], &os[4], &os[5], &os[6], &os[7], &os[8], &os[9], &os[10], &os[11], &os[12]) == 13);
	assert(os[0]->f == 65535);
	assert(os[1]->f == 65536);
	assert(os[2]->f == 1048575);
	assert(os[3]->f == 4294967291U);
	assert(os[4]->f == 1234);
	assert(os[5]->f == 127);
	assert(os[6]->f == 255);
	assert(os[7]->f == 80);
	assert(os[8]->f == 8080);
	assert(os[9]->f == 4294967297);
	assert(os[10]->f == 1099511627775);
	assert(os[11]->f >= 1.33);
	assert(os[11]->f <= 1.331);
	assert(os[12]->f == 1.444444);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
