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

struct b6b_zlib_deflate_data {
	mz_stream strm;
	mz_uint rem;
	size_t slen;
	int ok;
};

static void b6b_zlib_do_deflate(void *arg)
{
	struct b6b_zlib_deflate_data *data = (struct b6b_zlib_deflate_data *)arg;

	while (data->rem) {
		if (data->rem > B6B_DEFLATE_CHUNK_SZ)
			data->strm.avail_in = B6B_DEFLATE_CHUNK_SZ;
		else
			data->strm.avail_in = data->rem;

		if (mz_deflate(&data->strm, 0) != MZ_OK)
			return;

		data->rem = (mz_uint)data->slen - data->strm.total_in;
	}

	data->ok = 1;
}

static enum b6b_res b6b_zlib_deflate(struct b6b_interp *interp,
                                     struct b6b_obj *args,
                                     const int wbits)
{
	struct b6b_zlib_deflate_data data = {.strm = {0}};
	size_t dlen;
	struct b6b_obj *s, *l, *o;
	unsigned char *src, *dst;
	int level = MZ_DEFAULT_COMPRESSION;

	switch (b6b_proc_get_args(interp, args, "os|i", NULL, &s, &l)) {
		case 3:
			if ((l->i < INT_MIN) || (l->i > INT_MAX))
				return B6B_ERR;
			level = (int)l->i;

		case 2:
			break;

		default:
			return B6B_ERR;
	}

	if (s->slen > UINT32_MAX)
		return B6B_ERR;

	if (mz_deflateInit2(&data.strm,
	                    level,
	                    MZ_DEFLATED,
	                    wbits,
	                    9,
	                    MZ_DEFAULT_STRATEGY) != MZ_OK)
		return B6B_ERR;

	/* copy the buffer, since s->s may be freed after context switch */
	src = (unsigned char *)malloc(s->slen);
	if (!b6b_allocated(src)) {
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	dlen = (size_t)mz_deflateBound(&data.strm, (mz_ulong)s->slen);
	dst = (unsigned char *)malloc(dlen);
	if (!b6b_allocated(dst)) {
		free(src);
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	data.slen = s->slen;
	memcpy(src, s->s, data.slen);

	data.strm.next_in = src;
	data.strm.next_out = dst;
	data.strm.avail_out = (mz_uint32)dlen;

	data.rem = (mz_uint)s->slen;
	data.ok = 0;
	if (!b6b_offload(interp, b6b_zlib_do_deflate, &data) ||
	    !data.ok ||
	    (mz_deflate(&data.strm, MZ_FINISH) != MZ_STREAM_END)) {
		free(dst);
		free(src);
		mz_deflateEnd(&data.strm);
		return B6B_ERR;
	}

	free(src);
	o = b6b_str_new((char *)dst, (size_t)data.strm.total_out);
	mz_deflateEnd(&data.strm);
	if (b6b_unlikely(!o)) {
		free(dst);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_zlib_proc_deflate(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	return b6b_zlib_deflate(interp, args, -MZ_DEFAULT_WINDOW_BITS);
}

static enum b6b_res b6b_zlib_proc_compress(struct b6b_interp *interp,
                                           struct b6b_obj *args)
{
	return b6b_zlib_deflate(interp, args, MZ_DEFAULT_WINDOW_BITS);
}

struct b6b_zlib_inflate_data {
	mz_stream strm;
	size_t slen;
	size_t dlen;
	unsigned char *dst;
	mz_uint rem;
	int ok;
};

static void b6b_zlib_do_inflate(void *arg)
{
	struct b6b_zlib_inflate_data *data = (struct b6b_zlib_inflate_data *)arg;
	unsigned char *mdst;

	do {
		mdst = (unsigned char *)realloc(data->dst, data->dlen);
		if (!b6b_allocated(mdst))
			return;

		data->strm.next_out = mdst + (data->strm.next_out - data->dst);
		data->dst = mdst;
		data->strm.avail_out += B6B_INFLATE_OUT_CHUNK_SZ;

		if (data->rem > B6B_INFLATE_IN_CHUNK_SZ)
			data->strm.avail_in = B6B_INFLATE_IN_CHUNK_SZ;
		else
			data->strm.avail_in = data->rem;

		switch (mz_inflate(&data->strm, 0))  {
			case MZ_OK:
				break;

			case MZ_STREAM_END:
				data->ok = 1;
				return;

			default:
				return;
		}

		data->rem = (mz_uint)data->slen - data->strm.total_in;
		data->dlen += B6B_INFLATE_OUT_CHUNK_SZ;
	} while (1);
}

static enum b6b_res b6b_zlib_inflate(struct b6b_interp *interp,
                                     struct b6b_obj *args,
                                     const int wbits)
{
	struct b6b_zlib_inflate_data data = {.strm = {0}};
	struct b6b_obj *s, *o;
	unsigned char *src;

	if (!b6b_proc_get_args(interp, args, "os", NULL, &s))
		return B6B_ERR;

	if (s->slen > UINT32_MAX)
		return B6B_ERR;

	if (mz_inflateInit2(&data.strm, wbits) != MZ_OK)
		return B6B_ERR;

	src = (unsigned char *)malloc(s->slen);
	if (!b6b_allocated(src)) {
		mz_inflateEnd(&data.strm);
		return B6B_ERR;
	}

	data.dlen = B6B_INFLATE_OUT_CHUNK_SZ;
	data.slen = s->slen;
	memcpy(src, s->s, s->slen);

	data.strm.next_in = src;
	data.strm.next_out = NULL;
	data.strm.avail_out = 0;

	data.dst = NULL;
	data.rem = (mz_uint)s->slen;
	data.ok = 0;
	if (!b6b_offload(interp, b6b_zlib_do_inflate, &data) ||
	    !data.ok ||
	    (mz_inflate(&data.strm, MZ_FINISH) != MZ_STREAM_END)) {
		free(data.dst);
		free(src);
		mz_inflateEnd(&data.strm);
		return B6B_ERR;
	}

	free(src);
	o = b6b_str_new((char *)data.dst, (size_t)data.strm.total_out);
	mz_inflateEnd(&data.strm);
	if (b6b_unlikely(!o)) {
		free(data.dst);
		return B6B_ERR;
	}

	return b6b_return(interp, o);
}

static enum b6b_res b6b_zlib_proc_inflate(struct b6b_interp *interp,
                                          struct b6b_obj *args)
{
	return b6b_zlib_inflate(interp, args, -MZ_DEFAULT_WINDOW_BITS);
}

static enum b6b_res b6b_zlib_proc_decompress(struct b6b_interp *interp,
                                             struct b6b_obj *args)
{
	return b6b_zlib_inflate(interp, args, MZ_DEFAULT_WINDOW_BITS);
}

static const struct b6b_ext_obj b6b_zlib[] = {
	{
		.name = "deflate",
		.type = B6B_TYPE_STR,
		.val.s = "deflate",
		.proc = b6b_zlib_proc_deflate
	},
	{
		.name = "compress",
		.type = B6B_TYPE_STR,
		.val.s = "compress",
		.proc = b6b_zlib_proc_compress
	},
	{
		.name = "inflate",
		.type = B6B_TYPE_STR,
		.val.s = "inflate",
		.proc = b6b_zlib_proc_inflate
	},
	{
		.name = "decompress",
		.type = B6B_TYPE_STR,
		.val.s = "decompress",
		.proc = b6b_zlib_proc_decompress
	}
};
__b6b_ext(b6b_zlib);
