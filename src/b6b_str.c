/*
 * This file is part of b6b.
 *
 * Copyright 2017, 2020 Dima Krasner
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
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <wchar.h>

#include <b6b.h>

/* a strndup() that always copies len bytes without searching for \0 */
char *b6b_strndup(const char *s, const size_t len)
{
	char *s2 = (char *)malloc(len + 1);

	if (b6b_allocated(s2)) {
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
		o->flags = B6B_TYPE_STR;
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
		if (b6b_isspace(s[i]))
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

	if (o->flags & B6B_TYPE_STR)
		return 1;

	if (o->flags & B6B_TYPE_LIST) {
		o->s = NULL;

		b6b_list_foreach(o, li) {
			if (!b6b_as_str(li->o)) {
				free(o->s);
				return 0;
			}

			if (li->o->slen) {
				if (b6b_spaced(li->o->s, li->o->slen)) {
					if (o->s) { /* non-empty with whitespace */
						nlen = o->slen + li->o->slen + 3;
						s2 = (char *)realloc(o->s, nlen + 1);
						if (!b6b_allocated(s2)) {
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
					else { /* first, non-empty with whitespace */
						o->slen = li->o->slen + 2;
						o->s = (char *)malloc(o->slen + 1);
						if (!b6b_allocated(o->s))
							return 0;

						o->s[0] = '{';
						memcpy(&o->s[1], li->o->s, li->o->slen);
						o->s[o->slen - 1] = '}';
						o->s[o->slen] = '\0';
					}
				}
				else {
					if (o->s) { /* non-empty without whitespace */
						nlen = o->slen + li->o->slen + 1;
						s2 = (char *)realloc(o->s, nlen + 1);
						if (!b6b_allocated(s2)) {
							free(o->s);
							return 0;
						}

						s2[o->slen] = ' ';
						memcpy(&s2[o->slen + 1], li->o->s, li->o->slen);
						s2[nlen] = '\0';

						o->s = s2;
						o->slen = nlen;
					}
					else { /* first, non-empty witoput whitespace */
						o->s = b6b_strndup(li->o->s, li->o->slen);
						if (b6b_unlikely(!o->s))
							return 0;

						o->slen = li->o->slen;
					}
				}
			}
			else {
				if (o->s) { /* empty */
					nlen = o->slen + 3;
					s2 = (char *)realloc(o->s, nlen + 1);
					if (!b6b_allocated(s2)) {
						free(o->s);
						return 0;
					}

					s2[o->slen] = ' ';
					s2[o->slen + 1] = '{';
					s2[nlen - 1] = '}';
					s2[nlen] = '\0';

					o->s = s2;
					o->slen = nlen;
				}
				else { /* first, empty */
					o->s = (char *)malloc(3);
					if (!b6b_allocated(o->s))
						return 0;

					o->s[0] = '{';
					o->s[1] = '}';
					o->s[2] = '\0';
					o->slen = 2;
				}
			}
		}

		/* special case: empty list */
		if (!o->s) {
			o->s = (char *)malloc(3);
			if (!b6b_allocated(o->s))
				return 0;

			o->s[0] = '{';
			o->s[1] = '}';
			o->s[2] = '\0';

			o->slen = 2;
		}

		o->flags |= B6B_TYPE_STR;
		return 1;
	}

	if (o->flags & B6B_TYPE_FLOAT) {
		out = asprintf(&o->s, B6B_FLOAT_FMT, o->f);
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
	}
	else {
		out = asprintf(&o->s, B6B_INT_FMT, o->i);
		if (out < 0)
			return 0;

		if (out == 0) {
			free(o->s);
			return 0;
		}
	}

	o->slen = (size_t)out;
	o->flags |= B6B_TYPE_STR;
	return 1;
}

struct b6b_obj *b6b_str_decode(const char *s, size_t len)
{
	mbstate_t ps = {0};
	struct b6b_obj *l, *c;
	size_t out;

	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return 0;

	if (len) {
		do {
			out = mbrtowc(NULL, s, len, &ps);
			if ((out == 0) || (out == (size_t)-1) || (out == (size_t)-2)) {
				b6b_destroy(l);
				return NULL;
			}

			c = b6b_str_copy(s, out);
			if (b6b_unlikely(!c)) {
				b6b_destroy(l);
				return NULL;
			}

			if (b6b_unlikely(!b6b_list_add(l, c))) {
				b6b_destroy(c);
				b6b_destroy(l);
				return NULL;
			}

			b6b_unref(c);

			len -= out;
			/* if the entire string was converted, stop */
			if (!len)
				break;

			s += out;
		} while (1);
	}

	return l;
}

static enum b6b_res b6b_str_proc_len(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *s;

	if (b6b_proc_get_args(interp, args, "os", NULL, &s))
		return b6b_return_int(interp, (b6b_int)s->slen);

	return B6B_ERR;
}

static enum b6b_res b6b_str_proc_index(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s, *i;

	if (!b6b_proc_get_args(interp, args, "osi", NULL, &s, &i) ||
	    (i->i >= s->slen) ||
	    (i->i < 0))
	    return B6B_ERR;

	return b6b_return_str(interp, &s->s[i->i], 1);
}

static enum b6b_res b6b_str_proc_range(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s, *start, *end;
	size_t len;

	if (!b6b_proc_get_args(interp, args, "osii", NULL, &s, &start, &end) ||
	    (start->i < 0) ||
	    (start->i >= s->slen) ||
	    (end->i < 0) ||
	    (end->i >= s->slen) ||
	    (start->i > end->i))
	    return B6B_ERR;

	len = (size_t)(end->i - start->i);
	if (len == SIZE_MAX)
		return B6B_ERR;

	return b6b_return_str(interp,
	                      &s->s[(ptrdiff_t)start->i],
	                      len + 1);
}

static enum b6b_res b6b_str_proc_join(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *d, *l, *o;
	struct b6b_litem *li;
	char *s = NULL, *ms;
	size_t len = 0, mlen;

	if (!b6b_proc_get_args(interp, args, "osl", NULL, &d, &l))
		return B6B_ERR;

	li = b6b_list_first(l);
	if (!li)
		return b6b_return_str(interp, "", 0);

	if (!b6b_as_str(li->o))
		return B6B_ERR;

	len = li->o->slen;
	s = (char *)malloc(len + 1);
	if (!b6b_allocated(s))
		return B6B_ERR;

	memcpy(s, li->o->s, li->o->slen);

	for (li = b6b_list_next(li); li; li = b6b_list_next(li)) {
		if (!b6b_as_str(li->o)) {
			free(s);
			return B6B_ERR;
		}

		mlen = len + d->slen + li->o->slen;

		ms = (char *)realloc(s, mlen + 1);
		if (!b6b_allocated(ms)) {
			free(s);
			return B6B_ERR;
		}

		memcpy(ms + len, d->s, d->slen);
		memcpy(ms + len + d->slen, li->o->s, li->o->slen);

		s = ms;
		len = mlen;
	}

	s[len] = '\0';
	o = b6b_str_new(s, len);
	if (b6b_unlikely(!o)) {
		free(s);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_str_proc_split(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s, *d, *l, *o;
	const char *p, *end, *prev;

	if (!b6b_proc_get_args(interp, args, "oss", NULL, &s, &d))
	    return B6B_ERR;

	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return B6B_ERR;

	p = s->s;
	end = p + s->slen;
	prev = p;

	do {
		if (p == end) {
			if (d->slen && prev) {
				o = b6b_str_copy(prev, end - prev);
				if (b6b_unlikely(!o)) {
					b6b_destroy(l);
					return B6B_ERR;
				}

				if (b6b_unlikely(!b6b_list_add(l, o))) {
					b6b_destroy(o);
					b6b_destroy(l);
					return B6B_ERR;
				}

				b6b_unref(o);
			}
			break;
		}

		if (d->slen && memcmp(p, d->s, d->slen))
			++p;
		else {
			o = b6b_str_copy(prev, d->slen ? (p - prev) : 1);
			if (b6b_unlikely(!o)) {
				b6b_destroy(l);
				return B6B_ERR;
			}

			if (b6b_unlikely(!b6b_list_add(l, o))) {
				b6b_destroy(o);
				b6b_destroy(l);
				return B6B_ERR;
			}

			b6b_unref(o);

			p += d->slen ? d->slen : 1;
			prev = p;
		}
	} while (1);

	return b6b_return(interp, l);
}

static enum b6b_res b6b_str_proc_ord(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *s;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s) || (s->slen != 1))
	    return B6B_ERR;

	return b6b_return_int(interp, (b6b_int)(unsigned char)s->s[0]);
}

static enum b6b_res b6b_str_proc_chr(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *n, *s;
	char *buf;

	if (!b6b_proc_get_args(interp, args, "oi", NULL, &n) ||
	    (n->i < 0) ||
	    (n->i > UCHAR_MAX))
	    return B6B_ERR;

	buf = (char *)malloc(2);
	if (!b6b_allocated(buf))
		return B6B_ERR;

	s = b6b_str_new(buf, 1);
	if (b6b_unlikely(!s)) {
		free(buf);
		return B6B_ERR;
	}

	buf[0] = (char)n->i;
	buf[1] = '\0';
	return b6b_return(interp, s);
}

static enum b6b_res b6b_str_proc_expand(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *s, *o;
	char *s2;
	size_t i = 0, j = 0, last;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	s2 = (char *)malloc(s->slen + 1);
	if (!b6b_allocated(s2))
		return B6B_ERR;

	last = s->slen - 1;
	while (i < s->slen) {
		if (s->s[i] != '\\') {
			s2[j] = s->s[i];
			++i;
		}
		else if (j == last) {
			free(s2);
			b6b_return_str(interp, "bad escape", sizeof("bad escape") - 1);
			return B6B_ERR;
		}
		else {
			switch (s->s[i + 1]) {
				case '0':
					s2[j] = '\0';
					i += 2;
					break;

				case '\\':
					s2[j] = '\\';
					i += 2;
					break;

				case 'n':
					s2[j] = '\n';
					i += 2;
					break;

				case 't':
					s2[j] = '\t';
					i += 2;
					break;

				case 'r':
					s2[j] = '\r';
					i += 2;
					break;

				case 'x':
					if (i + 3 > last) {
						free(s2);
						b6b_return_str(interp,
						               "bad hex escape",
						               sizeof("bad hex escape") - 1);
						return B6B_ERR;
					}

					if ((s->s[i + 2] >= '0') && (s->s[i + 2] <= '9'))
						s2[j] = ((s->s[i + 2] - '0') << 4);
					else if ((s->s[i + 2] >= 'a') && (s->s[i + 2] <= 'f'))
						s2[j] = ((s->s[i + 2] - 'a' + 10) << 4);
					else if ((s->s[i + 2] >= 'A') && (s->s[i + 2] <= 'F'))
						s2[j] = ((s->s[i + 2] - 'A' + 10) << 4);
					else {
						free(s2);
						b6b_return_str(interp,
						               "bad hex digit",
						               sizeof("bad hex digit") - 1);
						return B6B_ERR;
					}

					if ((s->s[i + 3] >= '0') && (s->s[i + 3] <= '9'))
						s2[j] |= s->s[i + 3] - '0';
					else if ((s->s[i + 3] >= 'a') && (s->s[i + 3] <= 'f'))
						s2[j] |= s->s[i + 3] - 'a' + 10;
					else if ((s->s[i + 3] >= 'A') && (s->s[i + 3] <= 'F'))
						s2[j] |= s->s[i + 3] - 'A' + 10;
					else {
						free(s2);
						b6b_return_str(interp,
						               "bad hex digit",
						               sizeof("bad hex digit") - 1);
						return B6B_ERR;
					}

					i += 4;
					break;

				default:
					free(s2);
					b6b_return_str(interp,
					               "bad escape",
					               sizeof("bad escape") - 1);
					return B6B_ERR;
			}
		}

		++j;
	}

	s2[j] = '\0';
	o = b6b_str_new(s2, j);
	if (b6b_unlikely(!o)) {
		free(s2);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_str_proc_in(struct b6b_interp *interp,
                                    struct b6b_obj *args)
{
	struct b6b_obj *sub, *s;
	size_t i;

	if (!b6b_proc_get_args(interp, args, "oss", NULL, &sub, &s) || !sub->slen)
		return B6B_ERR;

	if (s->slen > sub->slen) {
		for (i = 0; i <= s->slen - sub->slen; ++i) {
			if (memcmp(&s->s[i], sub->s, sub->slen) == 0)
				return b6b_return_bool(interp, 1);
		}
	}
	else if (s->slen == sub->slen) {
		if (b6b_unlikely(!b6b_obj_hash(s)) || b6b_unlikely(!b6b_obj_hash(sub)))
			return B6B_ERR;


		return b6b_return_bool(interp, b6b_obj_eq(sub, s));
	}

	return b6b_return_bool(interp, 0);
}

static enum b6b_res b6b_str_proc_rtrim(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s;
	ssize_t i, j;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s) ||
	    (s->slen > SSIZE_MAX))
		return B6B_ERR;

	i = (ssize_t)s->slen;
	if (i) {
		j = i - 1;
		while (i >= 0) {
			if (!b6b_isspace(s->s[j]))
				break;
			--i;
			--j;
		}
	}

	return b6b_return_str(interp, s->s, (size_t)i);
}

static enum b6b_res b6b_str_proc_ltrim(struct b6b_interp *interp,
                                       struct b6b_obj *args)
{
	struct b6b_obj *s;
	size_t i;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	for (i = 0; i < s->slen; ++i) {
		if (!b6b_isspace(s->s[i]))
			break;
	}

	return b6b_return_str(interp, s->s + i, s->slen - i);
}

static const struct b6b_ext_obj b6b_str[] = {
	{
		.name = "str.len",
		.type = B6B_TYPE_STR,
		.val.s = "str.len",
		.proc = b6b_str_proc_len
	},
	{
		.name = "str.index",
		.type = B6B_TYPE_STR,
		.val.s = "str.index",
		.proc = b6b_str_proc_index
	},
	{
		.name = "str.range",
		.type = B6B_TYPE_STR,
		.val.s = "str.range",
		.proc = b6b_str_proc_range
	},
	{
		.name = "str.join",
		.type = B6B_TYPE_STR,
		.val.s = "str.join",
		.proc = b6b_str_proc_join
	},
	{
		.name = "str.split",
		.type = B6B_TYPE_STR,
		.val.s = "str.split",
		.proc = b6b_str_proc_split
	},
	{
		.name = "str.ord",
		.type = B6B_TYPE_STR,
		.val.s = "str.ord",
		.proc = b6b_str_proc_ord
	},
	{
		.name = "str.chr",
		.type = B6B_TYPE_STR,
		.val.s = "str.chr",
		.proc = b6b_str_proc_chr
	},
	{
		.name = "str.expand",
		.type = B6B_TYPE_STR,
		.val.s = "str.expand",
		.proc = b6b_str_proc_expand
	},
	{
		.name = "str.in",
		.type = B6B_TYPE_STR,
		.val.s = "str.in",
		.proc = b6b_str_proc_in
	},
	{
		.name = "rtrim",
		.type = B6B_TYPE_STR,
		.val.s = "rtrim",
		.proc = b6b_str_proc_rtrim
	},
	{
		.name = "ltrim",
		.type = B6B_TYPE_STR,
		.val.s = "ltrim",
		.proc = b6b_str_proc_ltrim
	}
};
__b6b_ext(b6b_str);
