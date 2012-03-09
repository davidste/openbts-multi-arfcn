/*
 * sigvec.c
 *
 * Complex signal processing vectors
 *
 * Copyright (C) 2012  Thomas Tsou <ttsou@vt.edu>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "sigvec.h"

/*
 * Memory alignment size if requested. SSE aligned loads require memory
 * aligned at 16 byte boundaries. This is only relevant for certain
 * vectors such as filter taps, which will always be accessed from a
 * known, aligned location - such as 0.
 */
#define ALIGN_SZ		16

static void cxvec_init(struct cxvec *vec, int len, int buf_len,
		       int start, cmplx *buf, int flags)
{
	vec->len = len;
	vec->buf_len = buf_len;
	vec->flags = flags;
	vec->start_idx = start;
	vec->buf = buf;
	vec->data = &buf[start];
}

/*! \brief Allocate and initialize a complex vector
 *  \param[in] len The buffer length in complex samples
 *  \param[in] start Starting index for useful data
 *  \param[in] buf Pointer to a pre-existing buffer
 *  \param[in] flags Flags for various vector attributes
 *
 *  If buf is NULL, then a buffer will be allocated.
 */
struct cxvec *cxvec_alloc(int len, int start, cmplx *buf, int flags)
{
	struct cxvec *vec;

	/* Disallow zero length data */
	if ((start >= len) || (len <= 0) || (start < 0)) {
		fprintf(stderr, "cxvec_alloc: invalid input\n");
		return NULL;
	}

	vec = (struct cxvec *) malloc(sizeof(struct cxvec));

	if (!buf) {
		if (flags & CXVEC_FLG_MEM_ALIGN)
			buf = (cmplx *) memalign(ALIGN_SZ, sizeof(cmplx) * len);
		else
			buf = (cmplx *) malloc(sizeof(cmplx) * len);
	}

	cxvec_init(vec, len - start, len, start, buf, flags);

	return vec;
}

/*! \brief Free a complex vector 
 *  \param[in] vec The complex vector to free
 *
 *  If the buffer exists, it will be freed. Set the buffer pointer to NULL if
 *  this is not desired behaviour. Memory allocated from memalign() can be
 *  recalimed with free() using glibc. Check memory reclamation requirments
 *  for other systems.
 */
void cxvec_free(struct cxvec *vec)
{
	if (vec->buf)
		free(vec->buf);

	free(vec);
}

/*! \brief Set all values of a complex vector to zero
 *  \param[in] vec The complex vector to zero
 *
 *  The entire buffer is set to zero including headroom.
 */
void cxvec_reset(struct cxvec *vec)
{
	memset(vec->buf, 0, vec->buf_len * sizeof(cmplx));
}

/*! \brief Deinterleave M complex vectors with forward loading
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 *  \param[in] m Number of channels
 *
 *  Deinterleave a complex vector from channel 0 to 'M-1'.
 */
int cxvec_deinterlv_fw(struct cxvec *in, struct cxvec **out, int m)
{
	int i, n;

	assert(!(in->len % m));

	for (i = 0; i < (in->len / m); i++) {
		for (n = 0; n < m; n++) {
			out[n]->data[i] = in->data[i * m + n];
		}
	}

	return 0;
}

/*! \brief Deinterleave M complex vectors with reverse loading
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 *  \param[in] m Number of channels
 *
 *  Deinterleave a complex vector from channel 'M-1' to 0.
 */
int cxvec_deinterlv_rv(struct cxvec *in, struct cxvec **out, int m)
{
	int i, n;

	assert(!(in->len % m));

	for (i = 0; i < (in->len / m); i++) {
		for (n = 0; n < m; n++) {
			out[m-1-n]->data[i] = in->data[i * m + n];
		}
	}

	return i;
}

/*! \brief Interleave M complex vectors
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 *  \param[in] m Number of channels
 */
int cxvec_interlv(struct cxvec **in, struct cxvec *out, int m)
{
	int i, n;

	for (i = 0; i < in[0]->len; i++) {
		for (n = 0; n < m; n++) {
			out->data[i * m + n] = in[n]->data[i];
		}
	}

	return i;
}

/*! \brief Reverse and complex conjugate a complex vector
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 */
int cxvec_rvrs_conj(struct cxvec *in, struct cxvec *out)
{
	int i;
	struct cxvec *rev = cxvec_alloc(in->len, 0, NULL, 0);

	for (i = 0; i < in->len; i++) {
		rev->data[i].real = in->data[in->len - 1 - i].real;
		rev->data[i].imag = -in->data[in->len - 1 - i].imag;
	}

	memcpy(out->data, rev->data, in->len * sizeof(cmplx));

	cxvec_free(rev);

	return 0;
}

/*! \brief Set the length of a set of complex vectors
 *  \param[in] vecs Set of complex vectors
 *  \param[in] m Number of complex vectors in the set
 *  \param[in] len Desired length 
 */
int cxvecs_set_len(struct cxvec **vecs, int m, int len)
{
	int i;

	if ((m < 0) || (len < 0)) {
		fprintf(stderr, "cxvecs_set_len: invalid input\n");
		return -1;
	}

	/* Valid length? */
	if ((vecs[0]->start_idx + len) > vecs[0]->buf_len) {
		fprintf(stderr, "cxvecs_set_len: invalid input\n");
		return -1;
	}

	for (i = 0; i < m; i++) {
		vecs[i]->len = len;
	}

	return 0;
}

/*! \brief Copy the contents of a complex vector to another
 *  \param[in] src Source complex vector 
 *  \param[out] dst m Destination complex vector
 *
 *  Vectors must be of identical length. Head and tail space are not copied.
 */
int cxvec_cp(struct cxvec *dst, struct cxvec *src)
{
	int i;

	if (src->len != dst->len) {
		fprintf(stderr, "cxvec_cp: invalid input\n");
		return -1;
	}

	memcpy(dst->data, src->data, sizeof(cmplx) * src->len);

	return src->len;
}

/*! \brief Subtract the contents of a complex vector from another
 *  \param[in] a Input complex vector 'A'
 *  \param[in] b Input complex vector 'B'
 *  \param[out] out Output complex vector
 *
 * Vector 'B' is subtracted from vector 'A' and the results are written to
 * the output vector. All vectors must be of identical length. 
 */
int cxvec_sub(struct cxvec *a, struct cxvec *b, struct cxvec *out)
{
	int i;

	if ((a->len != b->len) || (b->len != out->len)) {
		fprintf(stderr, "cxvec_sub: invalid input\n");
		return -1;
	}

	for (i = 0; i < a->len; i++) {
		out->data[i].real = a->data[i].real - b->data[i].real;
		out->data[i].imag = a->data[i].imag - b->data[i].imag;
	}

	return a->len;
}

/*! \brief Set the starting point for valid data in a set of complex vectors
 *  \param[in] vecs Set of complex vectors
 *  \param[in] m Number of complex vectors
 *  \param[in] idx Desired starting index
 *
 * Shift the amount of headroom for the purpose of feeding M channels into
 * M-path filterbanks where the tap sizes may differ from one filterbank to
 * another. Vector length is _not_ adjusted accordingly. Use with caution.
 */
int cxvecs_set_idx(struct cxvec **vecs, int m, int idx)
{
	int i;

	if ((m < 0) || (idx < 0)) {
		fprintf(stderr, "cxvecs_set_idx: invalid input\n");
		return -1;
	}

	for (i = 0; i < m; i++) {
		vecs[i]->start_idx = idx;
		vecs[i]->data = &vecs[i]->buf[idx];
	}

	return 0;
}

/*! \brief Decimate a complex vector by an integer value
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector
 *  \param[in] idx Index of first kept value
 *  \param[in] decim Decimation rate 
 *
 * Index should be between 0 and 'decim - 1'. 
 */
int cxvec_decim(struct cxvec *in, struct cxvec *out, int idx, int decim)
{
	int i;

	if ((decim <= 0) || (idx < 0) || (idx >= decim)) {
		fprintf(stderr, "cxvec_decim: invalid input\n");
		fprintf(stderr, "decim %i, idx %i\n", decim, idx);
		return -1;
	}

	if (in->len % decim) {
		fprintf(stderr, "cxvec_decim: invalid input\n");
		fprintf(stderr, "decim %i, in->len %i\n", decim, in->len);
		return -1;
	}

	for (i = 0; i < (in->len / decim); i++) {
		out->data[i] = in->data[decim * i + idx];
	}

	return (in->len / decim);
}

/*! \brief Shift the index of a complex vector based on convolution type
 *  \param[in] vec Complex input vector
 *  \param[in] h Complex vector (filter) to be convolved against
 *  \param[in] type Convolution span type
 *
 * Modify the index of a complex vector such that output of convolution
 * with another vector matches the desired convolution span type. Use with
 * caution.
 */
int cxvec_shft(struct cxvec *vec, struct cxvec *h, enum cxvec_conv_type type)
{
	int shft;

	switch (type) {
	case CONV_FULL_SPAN:
		shft = 0;
		break;
	case CONV_NO_DELAY:
		shft = h->len / 2;
		break;
	case CONV_OVERLAP_ONLY:
	default:
		fprintf(stderr, "cxvec_shft: Unsupported convolution type\n");
		return -1;
	}

	if ((vec->start_idx + shft) < (h->len - 1)) {
		fprintf(stderr, "cxvec_center: insufficent headroom to filter\n");
		return -1;
	}

	if ((vec->start_idx + shft) > (vec->buf_len)) {
		fprintf(stderr, "cxvec_center: insufficent buffer length to filter\n");
		return -1;
	}

	vec->start_idx += shft;
	vec->data = &vec->buf[vec->start_idx];

	return shft;
}

float cxvec_sinc(float x)
{
	if (x == 0.0f)
		return .9999999999;

	return sin(M_PIf * x) / (M_PIf * x);
}
