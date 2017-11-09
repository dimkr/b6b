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
	int occ[5] = {0, 0, 0, 0, 0};

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$randint 1 10}", 15) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f >= 1);
	assert(interp.fg->_->f <= 10);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$randint 1 1}", 14) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$randint 3 1}", 14) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$randint -10 -1}", 17) == B6B_OK);
	assert(b6b_as_float(interp.fg->_));
	assert(interp.fg->_->f >= -10);
	assert(interp.fg->_->f <= -1);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$randint -10 -10}", 18) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	assert(b6b_call_copy(&interp, "{$randint -8 -10}", 17) == B6B_ERR);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	do {
		assert(b6b_call_copy(&interp, "{$randint -2 2}", 15) == B6B_OK);
		assert(b6b_as_int(interp.fg->_));
		assert(interp.fg->_->i >= -2);
		assert(interp.fg->_->i <= 2);
		++occ[interp.fg->_->i + 2];
	} while (!occ[0] || !occ[1] || !occ[2] || !occ[3] || !occ[4]);
	b6b_interp_destroy(&interp);

	assert(b6b_interp_new_argv(&interp, 0, NULL, B6B_OPT_TRACE));
	occ[1] = occ[2] = 0;
	do {
		assert(b6b_call_copy(&interp, "{$randint 1 2}", 14) == B6B_OK);
		assert(b6b_as_int(interp.fg->_));
		assert((interp.fg->_->i == 1) || (interp.fg->_->i == 2));
		++occ[interp.fg->_->i];
	} while (!occ[1] || !occ[2]);
	b6b_interp_destroy(&interp);

	return EXIT_SUCCESS;
}
