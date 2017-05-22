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

/*
 * these two structs are not packed, because we want them natively aligned -
 * otherwise, on some architectures, the linker adds padding between structs and
 * we cannot scan the sections containing them as arrays
 */

struct b6b_ext_obj {
	const char *name;
	b6b_procf proc;
	b6b_delf del;
	union {
		const char *s;
		b6b_num n;
	} val;
	uint8_t type;
};

struct b6b_ext {
	const struct b6b_ext_obj *os;
	uint8_t n;
};

#define __b6b_ext_name(os) b6b_ext_ ##os
#define __b6b_ext(_os) \
	struct b6b_ext __attribute__((section("_b6b_ext"), used)) \
	__b6b_ext_name(_os) = { \
		.os = _os, \
		.n = sizeof(_os) / sizeof(_os[0]) \
	}

extern const struct b6b_ext __start__b6b_ext[];
#define b6b_ext_first __start__b6b_ext
extern const struct b6b_ext __stop__b6b_ext[];
#define b6b_ext_last __stop__b6b_ext

typedef int (*b6b_initf)(struct b6b_interp *);

#define __b6b_initp_name(_init) __ ##_init
#define __b6b_init(_func) \
	__attribute__((section("_b6b_init"), used)) b6b_initf __b6b_initp_name(_func) = _func

extern b6b_initf __start__b6b_init[];
#define b6b_init_first __start__b6b_init
extern b6b_initf __stop__b6b_init[];
#define b6b_init_last __stop__b6b_init
