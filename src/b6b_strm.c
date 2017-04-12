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
	unsigned char *buf, *nbuf;
	/* we read in B6B_STRM_BUFSIZ chunks, to improve efficiency of buffered
	 * streams */
	ssize_t len = B6B_STRM_BUFSIZ, out, more;
	int eof = 0;

	if (strm->flags & B6B_STRM_EOF)
		return B6B_OK;

	if (strm->flags & B6B_STRM_CLOSED)
		return B6B_ERR;

	buf = (unsigned char *)malloc((size_t)len + 1);
	if (b6b_unlikely(!buf))
		return B6B_ERR;

	out = strm->ops->read(interp, strm->priv, buf, len, &eof);
	if (out > 0) {
		do {
			len += B6B_STRM_BUFSIZ;
			nbuf = (unsigned char *)realloc(buf, len);
			if (b6b_unlikely(!nbuf)) {
				free(buf);
				return B6B_ERR;
			}

			more = strm->ops->read(interp,
			                       strm->priv,
			                       nbuf + out,
			                       B6B_STRM_BUFSIZ,
			                       &eof);
			if (more < 0) {
				free(nbuf);
				return B6B_ERR;
			}
			else
				out += more;

			buf = nbuf;

			if (eof) {
				strm->flags |= B6B_STRM_EOF;
				break;
			}
		} while (1);

		buf[out] = '\0';
		o = b6b_str_new((char *)buf, (size_t)out);
		if (b6b_unlikely(!o)) {
			free(buf);
			return B6B_ERR;
		}

		return b6b_return(interp, o);
	}

	free(buf);
	return (out == 0) ? B6B_OK : B6B_ERR;
}

static enum b6b_res b6b_strm_write(struct b6b_interp *interp,
                                   struct b6b_strm *strm,
                                   const unsigned char *buf,
                                   const size_t len)
{
	ssize_t out = 0, chunk;

	if (strm->flags & B6B_STRM_CLOSED)
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

static enum b6b_res b6b_strm_proc(struct b6b_interp *interp,
                                  struct b6b_obj *args)
{
	struct b6b_obj *o, *op, *s;
	unsigned int argc;

	argc = b6b_proc_get_args(interp, args, "o s |s", &o, &op, &s);

	if ((argc == 2) && (strcmp(op->s, "read") == 0))
		return b6b_strm_read(interp,
		                     (struct b6b_strm *)o->priv);
	else if ((argc == 3) && (strcmp(op->s, "write") == 0))
		return b6b_strm_write(interp,
		                      (struct b6b_strm *)o->priv,
		                      (const unsigned char *)s->s,
		                      s->slen);

	if (argc >= 2)
		b6b_return_fmt(interp, "bad strm op: %s", op->s);

	return B6B_ERR;
}

struct b6b_obj *b6b_strm_new(struct b6b_interp *interp,
                             struct b6b_strm *strm,
                             const char *type)
{
	struct b6b_obj *o;

	o = b6b_str_fmt("%s:%"PRIxPTR, type, (uintptr_t)strm->priv);
	if (b6b_unlikely(!o))
		return o;

	if (b6b_unlikely(!b6b_global(interp, o, o))) {
		b6b_destroy(o);
		return NULL;
	}

	o->priv = strm;
	o->proc = b6b_strm_proc;
	o->del = b6b_strm_del;

	return o;
}
