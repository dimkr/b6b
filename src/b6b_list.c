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

#include <b6b.h>

struct b6b_obj *b6b_list_new(void)
{
	struct b6b_obj *o;

	o = b6b_new();
	if (o) {
		TAILQ_INIT(&o->l);
		o->flags |= B6B_OBJ_LIST;
	}

	return o;
}

static void b6b_on_list_mod(struct b6b_obj *l)
{
	if (l->flags & B6B_OBJ_STR)
		free(l->s);

	l->flags &= ~(B6B_OBJ_NUM | B6B_OBJ_STR | B6B_OBJ_HASHED);
}

int b6b_list_add(struct b6b_obj *l, struct b6b_obj *o)
{
	struct b6b_litem *li;

	li = (struct b6b_litem *)malloc(sizeof(*li));
	if (b6b_unlikely(!li))
		return 0;

	li->o = b6b_ref(o);
	TAILQ_INSERT_TAIL(&l->l, li, ents);

	b6b_on_list_mod(l);
	return 1;
}

struct b6b_obj *b6b_list_pop(struct b6b_obj *l)
{
	struct b6b_obj *o = NULL;
	struct b6b_litem *li = TAILQ_LAST(&l->l, b6b_lhead);

	if (li) {
		b6b_list_remove(l, li);
		o = li->o;
		free(li);
		b6b_on_list_mod(l);
	}

	return o;
}

int b6b_as_list(struct b6b_obj *o)
{
	char *tok = NULL;
	struct b6b_litem *li;
	size_t i, tlen = 0;
	int nbrac = 0, nbrak = 0, trim = 0;

	if (!(o->flags & B6B_OBJ_LIST)) {
		if (!b6b_as_str(o))
			return 0;

		TAILQ_INIT(&o->l);

		for (i = 0; i <= o->slen; ++i) {
			switch (o->s[i]) {
				case '{':
					if ((++nbrac == 1) && !tok) {
						tok = &o->s[i];
						tlen = 1;
						trim = 1;
					}
					else
						++tlen;
					continue;

				case '}':
					--nbrac;
					++tlen;
					continue;

				case '[':
					if (!tok) {
						tok = &o->s[i];
						tlen = 1;
					}
					else
						++tlen;
					++nbrak;
					continue;

				case ']':
					--nbrak;
					++tlen;
					continue;

				case ' ':
				case '\t':
				case '\r':
				case '\n':
					if (nbrac || nbrak) {
						++tlen;
						continue;
					}
					break;

				case '\0':
					break;

				default:
					if (tok)
						++tlen;
					else {
						tok = &o->s[i];
						tlen = 1;
					}
					continue;
			}

			/* ignore empty tokens, unless wrapped with braces */
			if (!tlen)
				continue;

			li = (struct b6b_litem *)malloc(sizeof(*li));
			if (b6b_unlikely(!li))
				goto bail;

			if (trim)
				li->o = b6b_str_copy(&tok[1], tlen - 2);
			else
				li->o = b6b_str_copy(tok, tlen);
			if (b6b_unlikely(!li->o)) {
				free(li);
				goto bail;
			}

			TAILQ_INSERT_TAIL(&o->l, li, ents);

			tok = NULL;
			tlen = 0;
			trim = 0;
			nbrak = 0;
		}

		o->flags |= B6B_OBJ_LIST;
	}

	return 1;

bail:
	b6b_destroy_l(o);
	return 0;
}

unsigned int b6b_list_vparse(struct b6b_obj *l, const char *fmt, va_list ap)
{
	const char *p = fmt;
	struct b6b_litem *li;
	struct b6b_obj **o;
	int opt = 0;
	unsigned int n = 0;

	li = b6b_list_first(l);
	do {
		switch (*p) {
			case '|':
				opt = 1;

			case ' ':
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

		o = va_arg(ap, struct b6b_obj **);
		if (o) {
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

				case 'n':
					if (!b6b_as_num(li->o))
						return 0;
					break;

				case 'i':
					if (!b6b_as_num(li->o) || (roundf(li->o->n) != li->o->n))
						return 0;
					break;

				default:
					return 0;
			}

			*o = li->o;
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
	if (l) {
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
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_len(struct b6b_interp *interp,
                                      struct b6b_obj *args)
{
	struct b6b_obj *l;
	struct b6b_litem *li;
	b6b_num len = 0;

	if (b6b_proc_get_args(interp, args, "o l", NULL, &l)) {
		b6b_list_foreach(l, li)
			++len;

		return b6b_return_num(interp, len);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_append(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *l, *o;

	if (b6b_proc_get_args(interp, args, "o l o", NULL, &l, &o)) {
		if (b6b_list_add(l, o))
			return B6B_OK;
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_extend(struct b6b_interp *interp,
                                         struct b6b_obj *args)
{
	struct b6b_obj *l, *l2;
	struct b6b_litem *li;

	if (b6b_proc_get_args(interp, args, "o l l", NULL, &l, &l2)) {
		b6b_list_foreach(l2, li) {
			if (!b6b_list_add(l, li->o))
				return B6B_ERR;
		}

		return B6B_OK;
	}

	return B6B_ERR;
}

static enum b6b_res b6b_list_proc_index(struct b6b_interp *interp,
                                        struct b6b_obj *args)
{
	struct b6b_obj *l, *n;
	struct b6b_litem *li;
	b6b_num i;

	if (b6b_proc_get_args(interp, args, "o l n", NULL, &l, &n)) {
		li = b6b_list_first(l);
		if (!li)
			return B6B_ERR;

		for (i = 1; i < n->n; ++i) {
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
	b6b_num i;

	if (!b6b_proc_get_args(interp, args, "o l n n", NULL, &l, &start, &end))
		return B6B_ERR;

	if (b6b_unlikely(start->n < 0) ||
	    b6b_unlikely(end->n < 0) ||
	    b6b_unlikely(start->n >= end->n))
		return B6B_ERR;

	li = b6b_list_first(l);
	if (!li)
		return B6B_ERR;

	for (i = 1; i <= start->n; ++i) {
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
		if (i > end->n)
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

	if (!b6b_proc_get_args(interp, args, "o o l", NULL, &o, &l))
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
	struct b6b_obj *l, *n, *o;
	struct b6b_litem *li;
	b6b_num i;

	if (!b6b_proc_get_args(interp, args, "o l n", NULL, &l, &n) ||
	    b6b_unlikely(n->n < 0))
		return B6B_ERR;

	li = b6b_list_first(l);
	if (!li) {
		b6b_return_str(interp, "pop from {}", sizeof("pop from {}") - 1);
		return B6B_ERR;
	}

	for (i = 1; i <= n->n; ++i) {
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
		.type = B6B_OBJ_STR,
		.val.s = "list.new",
		.proc = b6b_list_proc_new
	},
	{
		.name = "list.len",
		.type = B6B_OBJ_STR,
		.val.s = "list.len",
		.proc = b6b_list_proc_len
	},
	{
		.name = "list.append",
		.type = B6B_OBJ_STR,
		.val.s = "list.append",
		.proc = b6b_list_proc_append
	},
	{
		.name = "list.extend",
		.type = B6B_OBJ_STR,
		.val.s = "list.extend",
		.proc = b6b_list_proc_extend
	},
	{
		.name = "list.index",
		.type = B6B_OBJ_STR,
		.val.s = "list.index",
		.proc = b6b_list_proc_index
	},
	{
		.name = "list.range",
		.type = B6B_OBJ_STR,
		.val.s = "list.range",
		.proc = b6b_list_proc_range
	},
	{
		.name = "list.in",
		.type = B6B_OBJ_STR,
		.val.s = "list.in",
		.proc = b6b_list_proc_in
	},
	{
		.name = "list.pop",
		.type = B6B_OBJ_STR,
		.val.s = "list.pop",
		.proc = b6b_list_proc_pop
	}
};
__b6b_ext(b6b_list);
