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

#include <b6b.h>

int main()
{
	struct b6b_interp interp;

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$gunzip}", 9) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$gunzip {}}", 12) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$gzip [$str.join {} [$range 0 1000]]]}",
	                     48) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 2894);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(
	                 &interp,
	                 "{$gunzip [$gzip [[$open /dev/zero ru] read 524288]]}",
	                 52) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 524288);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(
	              &interp,
	              "{$gunzip [$gzip [[$open /dev/urandom ru] read 131072]]}",
	              55) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 131072);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.join {} [$range 0 1000]]}",
	                     40) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.expand \\x1f\\x8b\\x08\\x00\\x26\\xb7\\x8b\\x5a\\x00\\x03\\x4b\\x4c\\x4a\\x4e\\x01\\x00\\x11\\xcd\\x82\\xed\\x04\\x00\\x00\\x00]}",
	                     120) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* gzip magic is wrong - gunzip does not verify this */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.expand \\x1f\\xff\\x08\\x00\\x26\\xb7\\x8b\\x5a\\x00\\x03\\x4b\\x4c\\x4a\\x4e\\x01\\x00\\x11\\xcd\\x82\\xed\\x04\\x00\\x00\\x00]}",
	                     120) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* uncompressed length is bigger than it should be - gunzip does not verify
	 * it */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.expand \\x1f\\x8b\\x08\\x00\\x26\\xb7\\x8b\\x5a\\x00\\x03\\x4b\\x4c\\x4a\\x4e\\x01\\x00\\x11\\xcd\\x82\\xed\\xff\\xff\\x00\\x00]}",
	                     120) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* CRC32 is incorrect - gunzip does not verify it */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.expand \\x1f\\x8b\\x08\\x00\\x26\\xb7\\x8b\\x5a\\x00\\x03\\x4b\\x4c\\x4a\\x4e\\x01\\x00\\x11\\xcd\\x82\\xff\\x04\\x00\\x00\\x00]}",
	                     120) == B6B_OK);
	assert(b6b_as_str(interp.fg->_));
	assert(interp.fg->_->slen == 4);
	assert(strcmp(interp.fg->_->s, "abcd") == 0);
	b6b_interp_destroy(&interp);

	/* there's nothing between the header and the footer */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.expand \\x1f\\x8b\\x08\\x00&\\xb7\\x8bZ\\x00\\x03\\x11\\xcd\\x82\\xed\\x04\\x00\\x00\\x00]}",
	                     90) == B6B_ERR);
	b6b_interp_destroy(&interp);

	/* there's nothing between the header and the footer and 1 footer byte is
	 * missing */
	assert(b6b_interp_new_argv(&interp, 0, NULL, 0));
	assert(b6b_call_copy(&interp,
	                     "{$gunzip [$str.expand \\x1f\\x8b\\x08\\x00&\\xb7\\x8bZ\\x00\\x03\\x11\\xcd\\x82\\xed\\x04\\x00\\x00]}",
	                     86) == B6B_ERR);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
