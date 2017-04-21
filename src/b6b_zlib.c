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

#include <limits.h>
#include <stdlib.h>

#include <b6b.h>

#include "miniz/miniz.h"
#include "miniz/miniz_tdef.h"

#define B6B_DEFLATE_CHUNK_SZ 64 * 1024
#define B6B_INFLATE_IN_CHUNK_SZ 64 * 1024
#define B6B_INFLATE_OUT_CHUNK_SZ 256 * 1024

static enum b6b_res b6b_zlib_proc_deflate(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	mz_stream strm = {0};
	size_t slen, dlen;
	struct b6b_obj *s, *l, *o;
	unsigned char *src, *dst;
	int level = MZ_DEFAULT_COMPRESSION;
	mz_uint rem;

	switch (b6b_proc_get_args(interp, args, "o s |i", NULL, &s, &l)) {
		case 3:
			if ((l->n < INT_MIN) || (l->n > INT_MAX))
				return B6B_ERR;
			level = (int)l->n;

		case 2:
			break;

		default:
			return B6B_ERR;
	}

	if (s->slen > UINT32_MAX)
		return B6B_ERR;

	if (mz_deflateInit(&strm, level) != MZ_OK)
		return B6B_ERR;

	/* copy the buffer, since s->s may be freed after context switch */
	src = (unsigned char *)malloc(s->slen);
	if (b6b_unlikely(!src)) {
		mz_deflateEnd(&strm);
		return B6B_ERR;
	}

	dlen = (size_t)mz_deflateBound(&strm, (mz_ulong)s->slen);
	dst = (unsigned char *)malloc(dlen);
	if (b6b_unlikely(!dst)) {
		free(src);
		mz_deflateEnd(&strm);
		return B6B_ERR;
	}

	slen = s->slen;
	memcpy(src, s->s, slen);

	strm.next_in = src;
	strm.next_out = dst;
	strm.avail_out = (mz_uint32)dlen;

	rem = (mz_uint)s->slen;
	do {
		if (rem > B6B_DEFLATE_CHUNK_SZ)
			strm.avail_in = B6B_DEFLATE_CHUNK_SZ;
		else
			strm.avail_in = rem;

		if (mz_deflate(&strm, 0) != MZ_OK) {
			free(dst);
			free(src);
			mz_deflateEnd(&strm);
			return B6B_ERR;
		}

		/* compression is CPU-intensive; switch to another thread */
		b6b_yield(interp);

		rem = (mz_uint)slen - strm.total_in;
	} while (rem);

	if (mz_deflate(&strm, MZ_FINISH) != MZ_STREAM_END) {
		free(dst);
		free(src);
		mz_deflateEnd(&strm);
		return B6B_ERR;
	}

	free(src);
	o = b6b_str_new((char *)dst, (size_t)strm.total_out);
	mz_deflateEnd(&strm);
	if (b6b_unlikely(!o)) {
		free(dst);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_zlib_proc_inflate(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	mz_stream strm = {0};
	size_t slen, dlen;
	struct b6b_obj *s, *o;
	unsigned char *src, *dst = NULL, *mdst;
	mz_uint rem;

	if (!b6b_proc_get_args(interp, args, "o s", NULL, &s))
		return B6B_ERR;

	if (s->slen > UINT32_MAX)
		return B6B_ERR;

	if (mz_inflateInit(&strm) != MZ_OK)
		return B6B_ERR;

	src = (unsigned char *)malloc(s->slen);
	if (b6b_unlikely(!src)) {
		mz_inflateEnd(&strm);
		return B6B_ERR;
	}

	dlen = B6B_INFLATE_OUT_CHUNK_SZ;
	slen = s->slen;
	memcpy(src, s->s, slen);

	strm.next_in = src;
	strm.next_out = NULL;
	strm.avail_out = 0;

	rem = (mz_uint)s->slen;
	do {
		mdst = (unsigned char *)realloc(dst, dlen);
		if (b6b_unlikely(!mdst)) {
			free(dst);
			free(src);
			mz_inflateEnd(&strm);
			return B6B_ERR;
		}

		strm.next_out = mdst + (strm.next_out - dst);
		dst = mdst;
		strm.avail_out += B6B_INFLATE_OUT_CHUNK_SZ;

		if (rem > B6B_INFLATE_IN_CHUNK_SZ)
			strm.avail_in = B6B_INFLATE_IN_CHUNK_SZ;
		else
			strm.avail_in = rem;

		switch (mz_inflate(&strm, 0))  {
			case MZ_OK:
				break;

			case MZ_STREAM_END:
				goto flush;

			default:
				free(dst);
				free(src);
				mz_inflateEnd(&strm);
				return B6B_ERR;
		}

		b6b_yield(interp);

		rem = (mz_uint)slen - strm.total_in;
		dlen += B6B_INFLATE_OUT_CHUNK_SZ;
	} while (1);

flush:
	if (mz_inflate(&strm, MZ_FINISH) != MZ_STREAM_END) {
		free(dst);
		free(src);
		mz_inflateEnd(&strm);
		return B6B_ERR;
	}

	free(src);
	o = b6b_str_new((char *)dst, (size_t)strm.total_out);
	mz_inflateEnd(&strm);
	if (b6b_unlikely(!o)) {
		free(dst);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static const struct b6b_ext_obj b6b_zlib[] = {
	{
		.name = "deflate",
		.type = B6B_OBJ_STR,
		.val.s = "deflate",
		.proc = b6b_zlib_proc_deflate
	},
	{
		.name = "inflate",
		.type = B6B_OBJ_STR,
		.val.s = "inflate",
		.proc = b6b_zlib_proc_inflate
	}
};
__b6b_ext(b6b_zlib);
