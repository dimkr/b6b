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

/* for asprintf() */
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#undef _GNU_SOURCE

#include <b6b.h>

/* a strndup() that always copies len bytes without searching for \0 */
static char *b6b_strndup(const char *s, const size_t len)
{
	char *s2 = (char *)malloc(len + 1);

	if (b6b_likely(s2)) {
		memcpy(s2, s, len);
		s2[len] = '\0';
	}

	return s2;
}

struct b6b_obj *b6b_str_new(char *s, const size_t len)
{
	struct b6b_obj *o = b6b_new();

	if (b6b_likely(o)) {
		o->s = s;
		o->slen = len;
		o->flags |= B6B_OBJ_STR;
	}

	return o;
}

struct b6b_obj *b6b_str_copy(const char *s, const size_t len)
{
	struct b6b_obj *o;
	char *s2 = b6b_strndup(s, len);

	if (b6b_unlikely(!s2))
		return NULL;

	o = b6b_str_new(s2, len);
	if (b6b_unlikely(!o))
		free(s2);

	return o;
}

struct b6b_obj *b6b_str_vfmt(const char *fmt, va_list ap)
{
	char *s;
	int len;

	len = vasprintf(&s, fmt, ap);
	if (b6b_unlikely(len < 0))
		return NULL;

	return b6b_str_new(s, (size_t)len);
}

struct b6b_obj *b6b_str_fmt(const char *fmt, ...)
{
	va_list ap;
	struct b6b_obj *s;

	va_start(ap, fmt);
	s = b6b_str_vfmt(fmt, ap);
	va_end(ap);

	return s;
}

static int b6b_spaced(const char *s, const size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i) {
		if ((s[i] == ' ') ||
		    (s[i] == '\t') ||
		    (s[i] == '\n') ||
		    (s[i] == '\r'))
			return 1;
	}

	return 0;
}

int b6b_as_str(struct b6b_obj *o)
{
	struct b6b_litem *li;
	char *s2;
	size_t nlen;
	int out, i;

	if (o->flags & B6B_OBJ_STR)
		return 1;

	if (o->flags & B6B_OBJ_LIST) {
		o->s = NULL;

		b6b_list_foreach(o, li) {
			if (!b6b_as_str(li->o)) {
				free(o->s);
				return 0;
			}

			if (b6b_spaced(li->o->s, li->o->slen)) {
				if (o->s) {
					nlen = o->slen + li->o->slen + 3;
					s2 = (char *)realloc(o->s, nlen + 1);
					if (b6b_unlikely(!s2)) {
						free(o->s);
						return 0;
					}

					s2[o->slen] = ' ';
					s2[o->slen + 1] = '{';
					memcpy(&s2[o->slen + 2], li->o->s, li->o->slen);
					s2[nlen - 1] = '}';
					s2[nlen] = '\0';

					o->s = s2;
					o->slen = nlen;
				}
				else {
					o->slen = li->o->slen + 2;
					o->s = (char *)malloc(o->slen + 1);
					if (b6b_unlikely(!o->s))
						return 0;

					o->s[0] = '{';
					memcpy(&o->s[1], li->o->s, li->o->slen);
					o->s[o->slen - 1] = '}';
					o->s[o->slen] = '\0';
				}
			}
			else {
				if (o->s) {
					nlen = o->slen + li->o->slen + 1;
					s2 = (char *)realloc(o->s, nlen + 1);
					if (b6b_unlikely(!s2)) {
						free(o->s);
						return 0;
					}

					s2[o->slen] = ' ';
					memcpy(&s2[o->slen + 1], li->o->s, li->o->slen);
					s2[nlen] = '\0';

					o->s = s2;
					o->slen = nlen;
				}
				else {
					o->s = b6b_strndup(li->o->s, li->o->slen);
					if (b6b_unlikely(!o->s))
						return 0;

					o->slen = li->o->slen;
				}
			}
		}

		/* special case: empty list */
		if (!o->s) {
			o->s = (char *)malloc(3);
			if (b6b_unlikely(!o->s))
				return 0;

			o->s[0] = '{';
			o->s[1] = '}';
			o->s[2] = '\0';

			o->slen = 2;
		}

		o->flags |= B6B_OBJ_STR;
	}
	else if (o->flags & B6B_OBJ_NUM) {
		out = asprintf(&o->s, B6B_NUM_FMT, o->n);
		if (out < 0)
			return 0;

		if (out == 0) {
			free(o->s);
			return 0;
		}

		/* strip trailing 0 and . */
		for (i = out - 1; i >= 0; --i) {
			if (o->s[i] == '0')
				--out;
			else {
				if (o->s[i] == '.')
					--out;

				o->s[out] = '\0';
				break;
			}
		}

		o->slen = (size_t)out;
		o->flags |= B6B_OBJ_STR;
	}

	return 1;
}

/* Jenkins's one-at-a-time hash */
uint32_t b6b_str_hash(const char *s, const size_t len)
{
	size_t i;
	uint32_t h = 0;

	for (i = 0; i < len; ++i) {
		h += s[i] + (h << 10);
		h ^= (h >> 6);
	}

	h += (h << 3);
	h ^= (h >> 11);
	h += (h << 15);
	return h;
}

static enum b6b_res b6b_str_proc_len(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *s;

	if (b6b_proc_get_args(interp, args, "o s", NULL, &s))
		return b6b_return_num(interp, (b6b_num)s->slen);

	return B6B_ERR;
}

static enum b6b_res b6b_str_proc_index(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s, *i;

	if (!b6b_proc_get_args(interp, args, "o s i", NULL, &s, &i) ||
	    (i->n >= s->slen))
	    return B6B_ERR;

	return b6b_return_str(interp, &s->s[(ptrdiff_t)i->n], 1);
}

static enum b6b_res b6b_str_proc_range(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s, *start, *end;

	if (!b6b_proc_get_args(interp, args, "o s i i", NULL, &s, &start, &end) ||
	    (start->n < 0) ||
	    (start->n >= s->slen) ||
	    (end->n < 0) ||
	    (end->n >= s->slen) ||
	    (start->n > end->n))
	    return B6B_ERR;

	return b6b_return_str(interp,
	                      &s->s[(ptrdiff_t)start->n],
	                      end->n - start->n);
}

static const struct b6b_ext_obj b6b_str[] = {
	{
		.name = "str.len",
		.type = B6B_OBJ_STR,
		.val.s = "str.len",
		.proc = b6b_str_proc_len
	},
	{
		.name = "str.index",
		.type = B6B_OBJ_STR,
		.val.s = "str.index",
		.proc = b6b_str_proc_index
	},
	{
		.name = "str.range",
		.type = B6B_OBJ_STR,
		.val.s = "str.range",
		.proc = b6b_str_proc_range
	}
};
__b6b_ext(b6b_str);
