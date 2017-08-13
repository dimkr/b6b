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
#include <stdarg.h>
#ifdef B6B_HAVE_VALGRIND
#	include <valgrind/memcheck.h>
#endif

#include <b6b.h>

struct b6b_obj *b6b_list_new(void)
{
	struct b6b_obj *o;

	o = b6b_new();
	if (o) {
		TAILQ_INIT(&o->l);
		o->flags = B6B_TYPE_LIST;
	}

	return o;
}

static int b6b_list_do_add(struct b6b_obj *l, struct b6b_obj *o)
{
	struct b6b_litem *li;

	li = (struct b6b_litem *)malloc(sizeof(*li));
	if (b6b_unlikely(!li))
		return 0;

	li->o = b6b_ref(o);
	TAILQ_INSERT_TAIL(&l->l, li, ents);

	return 1;
}

struct b6b_obj *b6b_list_build(struct b6b_obj *o, ...)
{
	va_list ap;
	struct b6b_obj *l;

	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return NULL;

	va_start(ap, o);
	while (o) {
		if (!b6b_list_do_add(l, o)) {
			va_end(ap);
			b6b_destroy(l);
			return NULL;
		}

		o = va_arg(ap, struct b6b_obj *);
	}
	va_end(ap);

	return l;
}

void b6b_list_flush(struct b6b_obj *l)
{
	if (l->flags & B6B_TYPE_STR)
		free(l->s);

	l->flags &= ~(B6B_TYPE_INT |
	              B6B_TYPE_FLOAT |
	              B6B_TYPE_STR |
	              B6B_OBJ_HASHED);

#ifdef B6B_HAVE_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(&l->s, sizeof(l->s));
	VALGRIND_MAKE_MEM_UNDEFINED(&l->i, sizeof(l->i));
	VALGRIND_MAKE_MEM_UNDEFINED(&l->f, sizeof(l->f));
	VALGRIND_MAKE_MEM_UNDEFINED(&l->hash, sizeof(l->hash));
#endif
}

int b6b_list_add(struct b6b_obj *l, struct b6b_obj *o)
{
	if (!b6b_list_do_add(l, o))
		return 0;

	b6b_list_flush(l);
	return 1;
}

int b6b_list_extend(struct b6b_obj *l, struct b6b_obj *l2)
{
	struct b6b_litem *li;

	b6b_list_foreach(l2, li) {
		if (b6b_unlikely(!b6b_list_do_add(l, li->o)))
			return 0;
	}

	b6b_list_flush(l);
	return 1;
}

struct b6b_obj *b6b_list_pop(struct b6b_obj *l, struct b6b_litem *li)
{
	struct b6b_obj *o;

	if (!li) {
		li = TAILQ_LAST(&l->l, b6b_lhead);
		if (!li)
			return NULL;
	}

	b6b_list_remove(l, li);
	o = li->o;
	free(li);
	b6b_list_flush(l);

	return o;
}

int b6b_as_list(struct b6b_obj *o)
{
	struct b6b_obj *s;
	const char *start, *end;
	size_t i, j, last;
	size_t nbrac, nbrak, skip;

	if (!(o->flags & B6B_TYPE_LIST)) {
		if (!b6b_as_str(o))
			return 0;

		TAILQ_INIT(&o->l);

		i = 0;
		last = o->slen - 1;
		while (i < o->slen) {
			skip = 0;
			switch (o->s[i]) {
				/* if a token is wrapped with braces, find the shortest
				 * substring wrapped with balanced braces, then strip one pair
				 * of braces */
				case '{':
					nbrac = 1;
					j = i + 1;
					start = &o->s[j];
					while (j <= last) {
						switch (o->s[j]) {
							case '{':
								++nbrac;
								break;

							case '}':
								if (!--nbrac)
									goto brac_end;
								break;
						}

						++j;
					} while (1);

					b6b_destroy_l(o);
					return 0;

brac_end:
					end = &o->s[j];
					skip = 2;
					break;

				case '[':
					/* if a token is wrapped with brackets, find the shortest
					 * substring wrapped with balanced brackets that does not
					 * contain a literal ] */
					nbrak = 1;
					nbrac = 0;
					start = &o->s[i];
					j = i + 1;
					while (j <= last) {
						switch (o->s[j]) {
							case '{':
								++nbrac;
								break;

							case '}':
								--nbrac;
								break;

							case '[':
								++nbrak;
								break;

							case ']':
								if (!--nbrak && !nbrac) {
									end = &o->s[j + 1];
									goto brak_end;
								}
								break;
						}

						++j;
					} while (1);

					b6b_destroy_l(o);
					return 0;

brak_end:
					skip = 0;
					break;

				case ' ':
				case '\t':
				case '\r':
				case '\n':
					/* skip whitespace between tokens */
					++i;
					continue;

				default:
					/* otherwise, if it's a non-whitespace character that isn't
					 * { or [, this token ends at the nearest whitespace
					 * character or at the end of the string */
					for (j = i + 1;
					     (j != o->slen) &&
					     (o->s[j] != ' ') &&
					     (o->s[j] != '\t') &&
					     (o->s[j] != '\n') &&
					     (o->s[j] != '\r');
					     ++j);

					start = &o->s[i];
					end = &o->s[j];
					skip = 1;
			}

			s = b6b_str_copy(start, end - start);
			if (b6b_unlikely(!s)) {
				b6b_destroy_l(o);
				return 0;
			}

			if (b6b_unlikely(!b6b_list_do_add(o, s))) {
				b6b_destroy(s);
				b6b_destroy_l(o);
				return 0;
			}

			b6b_unref(s);

			i += end - start + skip;
		}

		o->flags |= B6B_TYPE_LIST;
	}

	return 1;
}

unsigned int b6b_list_vparse(struct b6b_obj *l, const char *fmt, va_list ap)
{
	const char *p = fmt;
	struct b6b_litem *li;
	void *arg;
	int opt = 0;
	unsigned int n = 0;

	li = b6b_list_first(l);
	do {
		switch (*p) {
			case '|':
				opt = 1;
				++p;
				continue;

			case '\0':
				/* if we reached the format string end and the last argument,
				 * it's OK */
				if (!li)
					return n;

				return 0;
		}

		if (!li) {
			/* if we reached the list end but the next argument is optional,
			 * it's OK too */
			if (opt)
				return n;
			return 0;
		}

		arg = va_arg(ap, void *);
		if (arg) {
			switch (*p) {
				case 'o':
					break;

				case 's':
					if (!b6b_as_str(li->o))
						return 0;
					break;

				case 'l':
					if (!b6b_as_list(li->o))
						return 0;
					break;

				case 'i':
					if (!b6b_as_int(li->o))
						return 0;
					break;

				case 'f':
					if (!b6b_as_float(li->o))
						return 0;
					break;

				case '*':
					*(struct b6b_litem **)arg = li;
					return n + 1;

				default:
					return 0;
			}

			*(struct b6b_obj **)arg = li->o;
		}

		li = b6b_list_next(li);
		++p;
		++n;
	} while (1);

	return 0;
}

unsigned int b6b_list_parse(struct b6b_obj *l, const char *fmt, ...)
{
	va_list ap;
	unsigned int argc;

	va_start(ap, fmt);
	argc = b6b_list_vparse(l, fmt, ap);
	va_end(ap);
	return argc;
}

static enum b6b_res b6b_list_proc_new(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *l;
	struct b6b_litem *li;

	l = b6b_list_new();
	if (b6b_likely(l)) {
		li = b6b_list_first(args);
		do {
			li = b6b_list_next(li);
			if (!li)
				break;

			if (b6b_unlikely(!b6b_list_add(l, li->o))) {
				b6b_destroy(l);
				return B6B_ERR;
			}
		} while (1);

		return b6b_return(interp, l);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_len(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *l;
	struct b6b_litem *li;
	b6b_int len = 0;

	if (b6b_proc_get_args(interp, args, "ol", NULL, &l)) {
		b6b_list_foreach(l, li)
			++len;

		return b6b_return_int(interp, len);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_append(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *l, *o;

	if (b6b_proc_get_args(interp, args, "olo", NULL, &l, &o)) {
		if (b6b_list_add(l, o))
			return b6b_return(interp, b6b_ref(l));
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_extend(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *l, *l2;

	if (b6b_proc_get_args(interp, args, "oll", NULL, &l, &l2) &&
	    b6b_list_extend(l, l2))
		return B6B_OK;

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_index(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *l, *i;
	struct b6b_litem *li;
	b6b_int j;

	if (b6b_proc_get_args(interp, args, "oli", NULL, &l, &i)) {
		li = b6b_list_first(l);
		if (!li)
			return B6B_ERR;

		for (j = 0; j < i->i; ++j) {
			li = b6b_list_next(li);
			if (!li)
				return B6B_ERR;
		}

		return b6b_return(interp, b6b_ref(li->o));
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_range(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *l, *r, *start, *end;
	struct b6b_litem *li;
	b6b_int i;

	if (!b6b_proc_get_args(interp, args, "olii", NULL, &l, &start, &end))
		return B6B_ERR;

	if (b6b_unlikely(start->i < 0) ||
	    b6b_unlikely(end->i < 0) ||
	    b6b_unlikely(start->i >= end->i))
		return B6B_ERR;

	li = b6b_list_first(l);
	if (!li)
		return B6B_ERR;

	for (i = 1; i <= start->i; ++i) {
		li = b6b_list_next(li);
		if (!li)
			return B6B_ERR;
	}

	r = b6b_list_new();
	if (b6b_unlikely(!r))
		return B6B_ERR;

	do {
		if (b6b_unlikely(!b6b_list_add(r, li->o))) {
			b6b_destroy(r);
			return B6B_ERR;
		}

		++i;
		if (i > end->i)
			break;

		li = b6b_list_next(li);
		if (!li) {
			b6b_destroy(r);
			return B6B_ERR;
		}
	} while (1);

	return b6b_return(interp, r);
}

static enum b6b_res b6b_list_proc_in(struct b6b_interp *interp,
                                     struct b6b_obj *args)
{
	struct b6b_obj *l, *o;
	struct b6b_litem *li;
	int eq;

	if (!b6b_proc_get_args(interp, args, "ool", NULL, &o, &l))
		return B6B_ERR;

	b6b_list_foreach(l, li) {
		if (b6b_unlikely(!b6b_obj_eq(o, li->o, &eq)))
			return B6B_ERR;

		if (eq)
			return b6b_return_true(interp);
	}

	return b6b_return_false(interp);
}

static enum b6b_res b6b_list_proc_pop(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *l, *i, *o;
	struct b6b_litem *li;
	b6b_int j;

	if (!b6b_proc_get_args(interp, args, "oli", NULL, &l, &i) ||
	    b6b_unlikely(i->i < 0))
		return B6B_ERR;

	li = b6b_list_first(l);
	if (!li) {
		b6b_return_str(interp, "pop from {}", sizeof("pop from {}") - 1);
		return B6B_ERR;
	}

	for (j = 1; j <= i->i; ++j) {
		li = b6b_list_next(li);
		if (!li)
			return B6B_ERR;
	}

	b6b_list_remove(l, li);
	o = li->o;
	free(li);
	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_list[] = {
	{
		.name = "list.new",
		.type = B6B_TYPE_STR,
		.val.s = "list.new",
		.proc = b6b_list_proc_new
	},
	{
		.name = "list.len",
		.type = B6B_TYPE_STR,
		.val.s = "list.len",
		.proc = b6b_list_proc_len
	},
	{
		.name = "list.append",
		.type = B6B_TYPE_STR,
		.val.s = "list.append",
		.proc = b6b_list_proc_append
	},
	{
		.name = "list.extend",
		.type = B6B_TYPE_STR,
		.val.s = "list.extend",
		.proc = b6b_list_proc_extend
	},
	{
		.name = "list.index",
		.type = B6B_TYPE_STR,
		.val.s = "list.index",
		.proc = b6b_list_proc_index
	},
	{
		.name = "list.range",
		.type = B6B_TYPE_STR,
		.val.s = "list.range",
		.proc = b6b_list_proc_range
	},
	{
		.name = "list.in",
		.type = B6B_TYPE_STR,
		.val.s = "list.in",
		.proc = b6b_list_proc_in
	},
	{
		.name = "list.pop",
		.type = B6B_TYPE_STR,
		.val.s = "list.pop",
		.proc = b6b_list_proc_pop
	}
};
__b6b_ext(b6b_list);
