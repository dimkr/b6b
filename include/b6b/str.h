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

#include <sys/types.h>
#include <stdarg.h>

__attribute__((nonnull(1)))
struct b6b_obj *b6b_str_new(char *s, const size_t len);

__attribute__((nonnull(1)))
struct b6b_obj *b6b_str_copy(const char *s, const size_t len);
struct b6b_obj *b6b_str_vfmt(const char *fmt, va_list ap);

__attribute__((format(printf, 1, 2)))
struct b6b_obj *b6b_str_fmt(const char *fmt, ...);

int b6b_as_str(struct b6b_obj *o);

__attribute__((nonnull(1)))
uint32_t b6b_str_hash(const char *s, const size_t len);

struct b6b_obj *b6b_str_decode(const char *s, size_t len);

static inline int b6b_isspace(const char c)
{
	return ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'));
}
