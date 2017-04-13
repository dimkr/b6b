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
#include <string.h>

#include <b6b.h>

static void b6b_strm_close(struct b6b_strm *strm)
{
	if (!(strm->flags & B6B_STRM_CLOSED)) {
		if (strm->ops->close)
			strm->ops->close(strm->priv);
		strm->flags |= (B6B_STRM_EOF | B6B_STRM_CLOSED);
	}
}

void b6b_strm_destroy(struct b6b_strm *strm)
{
	b6b_strm_close(strm);
	free(strm);
}

static void b6b_strm_del(void *priv)
{
	b6b_strm_destroy((struct b6b_strm *)priv);
}

static enum b6b_res b6b_strm_read(struct b6b_interp *interp,
                                  struct b6b_strm *strm)
{
	struct b6b_obj *o;
	unsigned char *buf = NULL, *nbuf;
	/* we read in B6B_STRM_BUFSIZ chunks, to improve efficiency of buffered
	 * streams */
	ssize_t len = B6B_STRM_BUFSIZ + 1, out = 0, more;
	int eof = 0, again = 1;

	if (strm->flags & B6B_STRM_EOF)
		return B6B_OK;

	if ((strm->flags & B6B_STRM_CLOSED) || !strm->ops->read)
		return B6B_ERR;

	do {
		nbuf = (unsigned char *)realloc(buf, len);
		if (b6b_unlikely(!nbuf)) {
			free(buf);
			return B6B_ERR;
		}

		more = strm->ops->read(interp,
		                       strm->priv,
		                       nbuf + out,
		                       B6B_STRM_BUFSIZ,
		                       &eof,
		                       &again);
		if (more < 0) {
			free(nbuf);
			return B6B_ERR;
		}

		out += more;
		buf = nbuf;

		if (eof) {
			strm->flags |= B6B_STRM_EOF;
			break;
		}
		else if (!more || !again)
			break;

		len += B6B_STRM_BUFSIZ;
	} while (1);

	buf[out] = '\0';
	o = b6b_str_new((char *)buf, (size_t)out);
	if (b6b_unlikely(!o)) {
		free(buf);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_strm_write(struct b6b_interp *interp,
                                   struct b6b_strm *strm,
                                   const unsigned char *buf,
                                   const size_t len)
{
	ssize_t out = 0, chunk;

	if ((strm->flags & B6B_STRM_CLOSED) || !strm->ops->write)
		return B6B_ERR;

	while ((size_t)out < len) {
		chunk = strm->ops->write(interp, strm->priv, buf + out, len - out);
		if (chunk < 0)
			return B6B_ERR;

		if (!chunk)
			break;

		out += chunk;
	}

	return b6b_return_num(interp, (b6b_num)out);
}

static enum b6b_res b6b_strm_accept(struct b6b_interp *interp,
                                   struct b6b_strm *strm)
{
	struct b6b_obj *l, *o;

	if ((strm->flags & B6B_STRM_CLOSED) || !strm->ops->accept)
		return B6B_ERR;

	l = b6b_list_new();
	if (b6b_unlikely(!l))
		return B6B_ERR;

	do {
		if (!strm->ops->accept(interp, strm->priv, &o)) {
			b6b_destroy(l);
			return B6B_ERR;
		}

		if (!o)
			break;

		if (b6b_unlikely(!b6b_list_add(l, o))) {
			b6b_destroy(o);
			b6b_destroy(l);
			return B6B_ERR;
		}

		b6b_unref(o);
	} while (1);

	return b6b_return(interp, l);
}

static enum b6b_res b6b_strm_peer(struct b6b_interp *interp,
                                  struct b6b_strm *strm)
{
	struct b6b_obj *o;

	if (strm->ops->peer) {
		o = strm->ops->peer(interp, strm->priv);
		if (o)
			return b6b_return(interp, o);
	}

	return B6B_ERR;
}

static enum b6b_res b6b_strm_proc(struct b6b_interp *interp,
                                  struct b6b_obj *args)
{
	struct b6b_obj *o, *op, *s;
	unsigned int argc;

	argc = b6b_proc_get_args(interp, args, "o s |s", &o, &op, &s);

	switch (argc) {
		case 2:
			if (strcmp(op->s, "read") == 0)
				return b6b_strm_read(interp, (struct b6b_strm *)o->priv);
			if (strcmp(op->s, "accept") == 0)
				return b6b_strm_accept(interp, (struct b6b_strm *)o->priv);
			else if (strcmp(op->s, "peer") == 0)
				return b6b_strm_peer(interp, (struct b6b_strm *)o->priv);
			break;

		case 3:
			if (strcmp(op->s, "write") == 0)
				return b6b_strm_write(interp,
				                      (struct b6b_strm *)o->priv,
				                      (const unsigned char *)s->s,
				                      s->slen);
			break;
	}

	if (argc >= 2)
		b6b_return_fmt(interp, "bad strm op: %s", op->s);

	return B6B_ERR;
}

static struct b6b_obj *b6b_strm_new(struct b6b_interp *interp,
                                    struct b6b_obj *o,
                                    struct b6b_strm *strm)
{
	if (b6b_unlikely(o)) {
		if (b6b_unlikely(!b6b_global(interp, o, o))) {
			b6b_destroy(o);
			return NULL;
		}

		o->priv = strm;
		o->proc = b6b_strm_proc;
		o->del = b6b_strm_del;
	}

	return o;
}

struct b6b_obj *b6b_strm_copy(struct b6b_interp *interp,
                              struct b6b_strm *strm,
                              const char *s,
                              const size_t len)
{
	return b6b_strm_new(interp, b6b_str_copy(s, len), strm);
}

struct b6b_obj *b6b_strm_fmt(struct b6b_interp *interp,
                             struct b6b_strm *strm,
                             const char *type)
{
	return b6b_strm_new(interp,
	                    b6b_str_fmt("%s:%"PRIxPTR, type, (uintptr_t)strm),
	                    strm);
}
