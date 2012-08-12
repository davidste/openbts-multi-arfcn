/*
 * convolve_sse.c
 *
 * Intel SSE3 optimized complex convolution
 *
 * Copyright (C) 2012 Thomas Tsou <ttsou@vt.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * See the COPYING file in the main directory for details.
 */ 

#include <string.h>
#include <assert.h>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <stdio.h>

#include "convolve.h"

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) \
	(((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | ((fp0)))
#endif

/* Generic non-optimized complex-real convolution */
static void conv_real_generic(cmplx *x,
			      cmplx *h,
			      cmplx *y,
			      int h_len, int len)
{
	int i;

	memset(y, 0, len * sizeof(cmplx));

	for (i = 0; i < len; i++) {
		mac_real_vec_n(&x[i], h, &y[i], h_len);
	}
}

/* 4-tap SSE3 complex-real convolution */
static void conv_real_sse4(float *restrict x,
			   float *restrict h,
			   float *restrict y,
			   int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;

	union {
		__m128 m;
		float w[4] __attribute__((aligned(16)));
	} total;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m7 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2*i + 0]);
		m1 = _mm_loadu_ps(&x[2*i + 4]);
		m2 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m3 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));

		/* Quad multiply */
		m4 = _mm_mul_ps(m2, m7);
		m5 = _mm_mul_ps(m3, m7);

		/* Sum and store */
		m6 = _mm_hadd_ps(m4, m5);
		m0 = _mm_hadd_ps(m6, m6);

		total.m = m0;
		y[2*i + 0] = total.w[0];
		y[2*i + 1] = total.w[1];
	}
}

/* 8-tap SSE3 complex-real convolution */
static void conv_real_sse8(float *restrict x,
			   float *restrict h,
			   float *restrict y,
			   int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7, m8, m9;

	union {
		__m128 m;
		float w[4] __attribute__((aligned(16)));
	} total;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);

	m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
	m5 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2*i + 0]);
		m1 = _mm_loadu_ps(&x[2*i + 4]);
		m2 = _mm_loadu_ps(&x[2*i + 8]);
		m3 = _mm_loadu_ps(&x[2*i + 12]);

		m6 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m7 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));
		m8 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
		m9 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(1, 3, 1, 3));

		/* Quad multiply */
		m0 = _mm_mul_ps(m6, m4);
		m1 = _mm_mul_ps(m7, m4);
		m2 = _mm_mul_ps(m8, m5);
		m3 = _mm_mul_ps(m9, m5);

		/* Sum and store */
		m6 = _mm_add_ps(m0, m2);
		m7 = _mm_add_ps(m1, m3);
		m8 = _mm_hadd_ps(m6, m7);
		m9 = _mm_hadd_ps(m8, m8);

		total.m = m9;
		y[2*i + 0] = total.w[0];
		y[2*i + 1] = total.w[1];
	}
}

/* 12-tap SSE3 complex-real convolution */
static void conv_real_sse12(float *restrict x,
			    float *restrict h,
			    float *restrict y,
			    int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11, m12, m13, m14, m15;

	union {
		__m128 m;
		float w[4] __attribute__((aligned(16)));
	} total;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);
	m4 = _mm_load_ps(&h[16]);
	m5 = _mm_load_ps(&h[20]);

	m12 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
	m13 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
	m14 = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(0, 2, 0, 2));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2*i + 0]);
		m1 = _mm_loadu_ps(&x[2*i + 4]);
		m2 = _mm_loadu_ps(&x[2*i + 8]);
		m3 = _mm_loadu_ps(&x[2*i + 12]);

		m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m5 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));
		m6 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
		m7 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(1, 3, 1, 3));

		m0 = _mm_loadu_ps(&x[2*i + 16]);
		m1 = _mm_loadu_ps(&x[2*i + 20]);

		m8 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m9 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));

		/* Quad multiply */
		m0 = _mm_mul_ps(m4, m12);
		m1 = _mm_mul_ps(m5, m12);
		m2 = _mm_mul_ps(m6, m13);
		m3 = _mm_mul_ps(m7, m13);
		m4 = _mm_mul_ps(m8, m14);
		m5 = _mm_mul_ps(m9, m14);

		/* Sum and store */
		m8  = _mm_add_ps(m0, m2);
		m9  = _mm_add_ps(m1, m3);
		m10 = _mm_add_ps(m8, m4);
		m11 = _mm_add_ps(m9, m5);

		m2 = _mm_hadd_ps(m10, m11);
		m3 = _mm_hadd_ps(m2, m2);

		total.m = m3;
		y[2*i + 0] = total.w[0];
		y[2*i + 1] = total.w[1];
	}
}

/* 16-tap SSE3 complex-real convolution */
static void conv_real_sse16(float *restrict x,
			    float *restrict h,
			    float *restrict y,
			    int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11, m12, m13, m14, m15;

	union {
		__m128 m;
		float w[4] __attribute__((aligned(16)));
	} total;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);

	m4 = _mm_load_ps(&h[16]);
	m5 = _mm_load_ps(&h[20]);
	m6 = _mm_load_ps(&h[24]);
	m7 = _mm_load_ps(&h[28]);

	m12 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
	m13 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
	m14 = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(0, 2, 0, 2));
	m15 = _mm_shuffle_ps(m6, m7, _MM_SHUFFLE(0, 2, 0, 2));

	for (i = 0; i < in_len; i++) {
		/* Load (unaligned) input data */
		m0 = _mm_loadu_ps(&x[2*i + 0]);
		m1 = _mm_loadu_ps(&x[2*i + 4]);
		m2 = _mm_loadu_ps(&x[2*i + 8]);
		m3 = _mm_loadu_ps(&x[2*i + 12]);

		m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m5 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));
		m6 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
		m7 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(1, 3, 1, 3));

		m0 = _mm_loadu_ps(&x[2*i + 16]);
		m1 = _mm_loadu_ps(&x[2*i + 20]);
		m2 = _mm_loadu_ps(&x[2*i + 24]);
		m3 = _mm_loadu_ps(&x[2*i + 28]);

		m8  = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m9  = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));
		m10 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
		m11 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(1, 3, 1, 3));

		/* Quad multiply */
		m0 = _mm_mul_ps(m4, m12);
		m1 = _mm_mul_ps(m5, m12);
		m2 = _mm_mul_ps(m6, m13);
		m3 = _mm_mul_ps(m7, m13);

		m4 = _mm_mul_ps(m8, m14);
		m5 = _mm_mul_ps(m9, m14);
		m6 = _mm_mul_ps(m10, m15);
		m7 = _mm_mul_ps(m11, m15);

		/* Sum and store */
		m8  = _mm_add_ps(m0, m2);
		m9  = _mm_add_ps(m1, m3);
		m10 = _mm_add_ps(m4, m6);
		m11 = _mm_add_ps(m5, m7);

		m0 = _mm_add_ps(m8, m10);
		m1 = _mm_add_ps(m9, m11);
		m2 = _mm_hadd_ps(m0, m1);
		m3 = _mm_hadd_ps(m2, m2);

		total.m = m3;
		y[2*i + 0] = total.w[0];
		y[2*i + 1] = total.w[1];
	}
}

/* 20-tap SSE3 complex-real convolution */
static void conv_real_sse20(float *restrict x,
			    float *restrict h,
			    float *restrict y,
			    int in_len)
{
	int i;
	__m128 m0, m1, m2, m3, m4, m5, m6, m7;
	__m128 m8, m9, m10, m11, m12, m13, m14, m15;

	/* Load (aligned) filter taps */
	m0 = _mm_load_ps(&h[0]);
	m1 = _mm_load_ps(&h[4]);
	m2 = _mm_load_ps(&h[8]);
	m3 = _mm_load_ps(&h[12]);
	m4 = _mm_load_ps(&h[16]);
	m5 = _mm_load_ps(&h[20]);
	m6 = _mm_load_ps(&h[24]);
	m7 = _mm_load_ps(&h[28]);
	m8 = _mm_load_ps(&h[32]);
	m9 = _mm_load_ps(&h[36]);

	m11 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
	m12 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
	m13 = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(0, 2, 0, 2));
	m14 = _mm_shuffle_ps(m6, m7, _MM_SHUFFLE(0, 2, 0, 2));
	m15 = _mm_shuffle_ps(m8, m9, _MM_SHUFFLE(0, 2, 0, 2));

	for (i = 0; i < in_len; i++) {
		/* Multiply-accumulate first 12 taps */
		m0 = _mm_loadu_ps(&x[2*i + 0]);
		m1 = _mm_loadu_ps(&x[2*i + 4]);
		m2 = _mm_loadu_ps(&x[2*i + 8]);
		m3 = _mm_loadu_ps(&x[2*i + 12]);
		m4 = _mm_loadu_ps(&x[2*i + 16]);
		m5 = _mm_loadu_ps(&x[2*i + 20]);

		m6  = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m7  = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));
		m8  = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
		m9  = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(1, 3, 1, 3));
		m0  = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(0, 2, 0, 2));
		m1  = _mm_shuffle_ps(m4, m5, _MM_SHUFFLE(1, 3, 1, 3));

		m2 = _mm_mul_ps(m6, m11);
		m3 = _mm_mul_ps(m7, m11);
		m4 = _mm_mul_ps(m8, m12);
		m5 = _mm_mul_ps(m9, m12);
		m6 = _mm_mul_ps(m0, m13);
		m7 = _mm_mul_ps(m1, m13);

		m0  = _mm_add_ps(m2, m4);
		m1  = _mm_add_ps(m3, m5);
		m8  = _mm_add_ps(m0, m6);
		m9  = _mm_add_ps(m1, m7);

		/* Multiply-accumulate last 8 taps */
		m0 = _mm_loadu_ps(&x[2*i + 24]);
		m1 = _mm_loadu_ps(&x[2*i + 28]);
		m2 = _mm_loadu_ps(&x[2*i + 32]);
		m3 = _mm_loadu_ps(&x[2*i + 36]);

		m4 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(0, 2, 0, 2));
		m5 = _mm_shuffle_ps(m0, m1, _MM_SHUFFLE(1, 3, 1, 3));
		m6 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(0, 2, 0, 2));
		m7 = _mm_shuffle_ps(m2, m3, _MM_SHUFFLE(1, 3, 1, 3));

		m0 = _mm_mul_ps(m4, m14);
		m1 = _mm_mul_ps(m5, m14);
		m2 = _mm_mul_ps(m6, m15);
		m3 = _mm_mul_ps(m7, m15);

		m4  = _mm_add_ps(m0, m2);
		m5  = _mm_add_ps(m1, m3);

		/* Final sum and store */
		m0 = _mm_add_ps(m8, m4);
		m1 = _mm_add_ps(m9, m5);
		m2 = _mm_hadd_ps(m0, m1);
		m3 = _mm_hadd_ps(m2, m2);
		m4 = _mm_shuffle_ps(m3, m3, _MM_SHUFFLE(0, 3, 2, 1));

		_mm_store_ss(&y[2*i + 0], m3);
		_mm_store_ss(&y[2*i + 1], m4);
	}
}

/*! \brief Convolve two complex vectors 
 *  \param[in] in_vec Complex input signal
 *  \param[in] h_vec Complex filter taps (stored in reverse order)
 *  \param[out] out_vec Complex vector output
 *
 * Modified convole with filter length dependent SSE optimization.
 * See generic convole call for additional information.
 */
int cxvec_convolve(struct cxvec *restrict in_vec,
		   struct cxvec *restrict h_vec,
		   struct cxvec *restrict out_vec)
{
	int i;
	cmplx *x, *h, *y;
	void (*conv_func)(float *, float *, float *, int);

	if (in_vec->len < out_vec->len) { 
		fprintf(stderr, "convolve: Invalid vector length\n");
		return -1;
	}

	if (in_vec->flags & CXVEC_FLG_REAL_ONLY) {
		fprintf(stderr, "convolve: Input data must be complex\n");
		return -1;
	}

	if (!(h_vec->flags & (CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN))) {
		fprintf(stderr, "convolve: Taps must be real and aligned\n");
		return -1;
	}

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_sse4;
		break;
	case 8:
		conv_func = conv_real_sse8;
		break;
	case 12:
		conv_func = conv_real_sse12;
		break;
	case 16:
		conv_func = conv_real_sse16;
		break;
	case 20:
		conv_func = conv_real_sse20;
		break;
	default:
		conv_func = NULL;
	}

	x = in_vec->data;	/* input */
	h = h_vec->data;	/* taps */
	y = out_vec->data;	/* output */

	if (!conv_func) {
		conv_real_generic(&x[-(h_vec->len - 1)],
				  h,
				  y,
				  h_vec->len, out_vec->len);
	} else {
		conv_func((float *) &x[-(h_vec->len - 1)],
			  (float *) h,
			  (float *) y, out_vec->len);
	}

	return i;
}

/*! \brief Single output convolution 
 *  \param[in] in Pointer to complex input samples
 *  \param[in] h Complex vector filter taps
 *  \param[out] out Pointer to complex output sample
 *
 * Modified single output convole with filter length dependent SSE
 * optimization. See generic convole call for additional information.
 */
int single_convolve(cmplx *restrict in,
		    struct cxvec *restrict h_vec,
		    cmplx *restrict out)
{
	void (*conv_func)(float *, float *, float *, int);

	if (!(h_vec->flags & (CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN))) {
		fprintf(stderr, "convolve: Taps must be real and aligned\n");
		return -1;
	}

	switch (h_vec->len) {
	case 4:
		conv_func = conv_real_sse4;
		break;
	case 8:
		conv_func = conv_real_sse8;
		break;
	case 12:
		conv_func = conv_real_sse12;
		break;
	case 16:
		conv_func = conv_real_sse16;
		break;
	case 20:
		conv_func = conv_real_sse20;
		break;
	default:
		conv_func = NULL;
	}

	if (!conv_func) {
		conv_real_generic(&in[-(h_vec->len - 1)],
				  h_vec->data,
				  out,
				  h_vec->len, 1);
	} else {
		conv_func((float *) &in[-(h_vec->len - 1)],
			  (float *) h_vec->data,
			  (float *) out, 1);
	}

	return 1;
}
