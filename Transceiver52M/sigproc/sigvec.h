/*
 * sigvec.h
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

#ifndef _SIGVEC_H_
#define _SIGVEC_H_

#define CXVEC_FLG_REAL_ONLY	(1 << 0)
#define CXVEC_FLG_MEM_ALIGN	(1 << 1)

#define M_PIf			(3.14159265358979323846264338327f)

typedef struct cmplx {
	float real;
	float imag;
} cmplx;

struct cxvec {
	int len;
	int buf_len;
	int flags;
	int start_idx;
	cmplx *buf;
	cmplx *data;
	cmplx _buf[0];
};

enum cxvec_conv_type {
        CONV_FULL_SPAN,
        CONV_OVERLAP_ONLY,
        CONV_NO_DELAY,
};

/* Complex vectors */
struct cxvec *cxvec_alloc(int len, int start, cmplx *buf, int flags);
void cxvec_free(struct cxvec *vec);
void cxvec_reset(struct cxvec *vec);
int cxvec_rvrs(struct cxvec *in, struct cxvec *out);
int cxvec_cp(struct cxvec *dst, struct cxvec *src);
int cxvec_sub(struct cxvec *a, struct cxvec *b, struct cxvec *out);
int cxvec_decim(struct cxvec *in, struct cxvec *out, int idx, int decim);
int cxvec_shft(struct cxvec *vec, struct cxvec *h, enum cxvec_conv_type type);

/* Vector set operations */
int cxvecs_set_len(struct cxvec **vecs, int m, int len);
int cxvecs_set_idx(struct cxvec **vecs, int m, int idx);

/* Interleavers */
int cxvec_interlv(struct cxvec **in, struct cxvec *out, int chan_m);
int cxvec_deinterlv_fw(struct cxvec *in, struct cxvec **out, int chan_m);
int cxvec_deinterlv_rv(struct cxvec *in, struct cxvec **out, int chan_m);

/* Floating point utilities */
float cxvec_sinc(float x);

#endif /* _SIGVEC_H_ */
