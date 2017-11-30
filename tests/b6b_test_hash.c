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
#include <time.h>
#include <inttypes.h>

#include <b6b.h>

#define NHASHES 4 * 1024

static int hash_cmp(const void *a, const void *b)
{
	uint32_t m = *(uint32_t *)a, n = *(uint32_t *)b;

	if (m > n)
		return 1;

	if (m < n)
		return -1;

	return 0;
}

int main()
{
	unsigned int seed, i;
	uint32_t *hashes;
	unsigned char c;

	seed = (unsigned int)time(NULL);
	hashes = (uint32_t *)malloc(sizeof(uint32_t) * NHASHES);
	assert(hashes);

	c = (unsigned char)(rand_r(&seed) & 0xFF);
	hashes[0] = b6b_hash_init();
	b6b_hash_update(&hashes[0], &c, 1);
	b6b_hash_finish(&hashes[0]);

	for (i = 1; i < NHASHES; ++i) {
		c = (unsigned char)(rand_r(&seed) & 0xFF);
		hashes[i] = hashes[i - 1];
		b6b_hash_update(&hashes[i], &c, 1);
		b6b_hash_finish(&hashes[i]);
		assert(hashes[i] != hashes[i - 1]);
	}

	qsort(hashes, NHASHES, sizeof(hashes[0]), hash_cmp);

	for (i = 0; i < NHASHES; ++i) {
		assert(!bsearch(&hashes[i],
		                &hashes[i + 1],
		                NHASHES - i - 1,
		                sizeof(hashes[0]),
		                hash_cmp));
	}

	free(hashes);
	return EXIT_SUCCESS;
}
