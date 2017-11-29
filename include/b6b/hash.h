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

#include <inttypes.h>

static inline uint32_t b6b_hash_init(void)
{
	return 0;
}

static inline void b6b_hash_update(uint32_t *hash,
                                   const unsigned char *buf,
                                   const size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i) {
		*hash += buf[i] + (*hash << 10);
		*hash ^= (*hash >> 6);
	}
}

static inline void b6b_hash_finish(uint32_t *hash)
{
	*hash += (*hash << 3);
	*hash ^= (*hash >> 11);
	*hash += (*hash << 15);
}

__attribute__((nonnull(1)))
uint32_t b6b_hash(const unsigned char *buf, const size_t len);
