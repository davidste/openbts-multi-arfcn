/*
 * fft.c
 *
 * Fast Fourier transform 
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fftw3.h>

#include "sigvec.h"
#include "fft.h"

static fftwf_complex *fft_in;
static fftwf_complex *fft_out;
static fftwf_plan fft_plan;
static int fft_len;
static int init = 0;

/*! \brief Initialize FFT backend 
 *  \param[in] reverse FFT direction
 *  \param[in] m FFT length 
 *
 * If the reverse is non-NULL, then an inverse FFT will be used.
 */
int init_fft(int reverse, int m)
{
	int direction = FFTW_FORWARD;

	/* Only init once */
	if (init)
		return -1;

	if (reverse)
		direction = FFTW_BACKWARD;

	fft_in = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * m);
	fft_out = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * m);
	fft_plan = fftwf_plan_dft_1d(m, fft_in, fft_out,
				     direction, FFTW_MEASURE);
	fft_len = m;

	return 0;
}

/*! \brief Free FFT backend resources 
 */
void free_fft()
{
	fftwf_destroy_plan(fft_plan);
	fftwf_free(fft_in);
	fftwf_free(fft_out);
}

/*! \brief Iteratively run the FFT on a complex vector 
 *  \param[in] in Complex vector input
 *  \param[in] out Complex vector output
 *
 * The input length must be a multiple of the FFT length. Note that output
 * length is not verified. There is a copy to and from FFTW aligned memory.
 */
int cxvec_fft(struct cxvec *in, struct cxvec *out)
{
	int i;

	if (in->len % fft_len) {
		fprintf(stderr, "cxvec_fft: invalid input length\n");
		fprintf(stderr, "in->len %i, fft_len: fft_len %i\n",
			in->len, fft_len);
		return -1;
	}

	for (i = 0; i < (in->len / fft_len); i++) {
		memcpy(fft_in, &in->data[i * fft_len], fft_len * sizeof(cmplx));
		fftwf_execute(fft_plan);
		memcpy(&out->data[i * fft_len], fft_out, fft_len * sizeof(cmplx));
	}

	return 0;
}
