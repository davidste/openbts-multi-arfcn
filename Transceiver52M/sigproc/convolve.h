/*
 * convolve.h
 *
 * Complex convolution
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

#ifndef _CONVOLVE_H_
#define _CONVOLVE_H_

#include "sigvec.h"

/* Generic multiply and accumulate complex-real */
inline void mac_real(float complex *x, float complex *h, float complex *y)
{
	*y = *x * creal(*h);
}

/* Generic multiply and accumulate complex-complex */
inline void mac_cmplx(float complex *x, float complex *h, float complex *y)
{
	*y = *x * *h;
}

/* Generic vector complex-real multiply and accumulate */
inline void mac_real_vec_n(float complex *x, float complex *h,
			   float complex *y, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		mac_real(&x[i], &h[i], y);
	}
}

/* Generic vector complex-complex multiply and accumulate */
inline void mac_cmplx_vec_n(float complex *x, float complex *h,
			    float complex *y, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		mac_cmplx(&x[i], &h[i], y);
	}
}

int cxvec_convolve(struct cxvec *in, struct cxvec *h, struct cxvec *out);
int single_convolve(float complex *in, struct cxvec *h, float complex *out);

#endif /* _CONVOLVE_H_ */
