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

#define B6B_STRM_BUFSIZ (64 * 1024)

struct b6b_strm;

struct b6b_strm_ops {
	ssize_t (*peeksz)(struct b6b_interp *, void *);
	ssize_t (*read)(struct b6b_interp *,
	                void *,
	                unsigned char *,
	                const size_t,
	                int *,
	                int *);
	ssize_t (*write)(struct b6b_interp *,
	                 void *,
	                 const unsigned char *,
	                 const size_t);
	struct b6b_obj *(*peer)(struct b6b_interp *, void *);
	int (*accept)(struct b6b_interp *, void *, struct b6b_obj **);
	int (*fd)(void *);
	void (*close)(void *);
};

enum b6b_strm_flags {
	B6B_STRM_CLOSED = 1,
	B6B_STRM_EOF    = 1 << 1
};

struct b6b_strm {
	const struct b6b_strm_ops *ops;
	void *priv;
	uint8_t flags;
};

struct b6b_obj *b6b_strm_copy(struct b6b_interp *interp,
                              struct b6b_strm *strm,
                              const char *s,
                              const size_t len);
struct b6b_obj *b6b_strm_fmt(struct b6b_interp *interp,
                             struct b6b_strm *strm,
                             const char *type);
void b6b_strm_destroy(struct b6b_strm *strm);
