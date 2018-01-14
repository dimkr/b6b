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

#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

#include <b6b.h>

struct test_data {
	int i;
	unsigned int ui;
	long l;
	unsigned long ul;
	void *p;
	char c;
	unsigned char uc;
	short s;
	unsigned short us;
	int64_t i64;
	uint64_t ui64;
	float f;
	double d;
};

struct packed_test_data {
	int i;
	char c;
	unsigned int ui;
	unsigned char uc;
	long l;
	short s;
	unsigned long ul;
	unsigned short us;
	void *p;
	float f;
	int64_t i64;
	double d;
	uint64_t ui64;
} __attribute__((packed));

int main()
{
	struct b6b_interp interp;
	const struct test_data data = {
		.i = 65535,
		.ui = 65536,
		.l = 1048575,
		.ul = 4294967291LU,
		.p = (void *)(uintptr_t)1234,
		.c = 127,
		.uc = 255,
		.s = 80,
		.us = 8080,
		.i64 = 4294967297,
		.ui64 = 1099511627775,
		.f = 1.33,
		.d = 1.444444
	};
	const struct packed_test_data pdata = {
		.i = 65535,
		.ui = 65536,
		.l = 1048575,
		.ul = 4294967291LU,
		.p = (void *)(uintptr_t)1234,
		.c = 127,
		.uc = 255,
		.s = 80,
		.us = 8080,
		.i64 = 4294967297,
		.ui64 = 1099511627775,
		.f = 1.33,
		.d = 1.444444
	};

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$ffi.pack}", 11) == B6B_ERR);
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
	assert(b6b_call_copy(&interp, "{$ffi.pack . 100}", 17) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.pack iIlLpbBhHqQfd 65535 65536 1048575 4294967291 1234 127 255 80 8080 4294967297 1099511627775 1.33 1.444444}",
	                     116) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == sizeof(data));
	assert(memcmp(interp.fg->_->s,
	              &data,
	              sizeof(data) - offsetof(struct test_data, f)) == 0);
	assert(((const struct test_data *)interp.fg->_->s)->f == data.f);
	assert(((const struct test_data *)interp.fg->_->s)->d == data.d);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$ffi.pack .ibIBlhLHpfqdQ 65535 127 65536 255 1048575 80 4294967291 8080 1234 1.33 4294967297 1.444444 1099511627775}",
	                     117) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == sizeof(pdata));
	assert(memcmp(interp.fg->_->s,
	              &pdata,
	              sizeof(pdata) - offsetof(struct packed_test_data, f)) == 0);
	assert(((const struct packed_test_data *)interp.fg->_->s)->f == pdata.f);
	assert(((const struct packed_test_data *)interp.fg->_->s)->d == pdata.d);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
