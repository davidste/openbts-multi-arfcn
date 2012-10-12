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

struct fft_hdl {
	struct cxvec *fft_in;
	struct cxvec *fft_out;
	fftwf_plan fft_plan;
};

/*! \brief Initialize FFT backend 
 *  \param[in] reverse FFT direction
 *  \param[in] m FFT length 
 *  \param[in] istride input stride count
 *  \param[in] ostride output stride count
 *  \param[in] in input buffer (FFTW aligned)
 *  \param[in] out output buffer (FFTW aligned)
 *  \param[in] ooffset initial offset into output buffer
 *
 * If the reverse is non-NULL, then an inverse FFT will be used. This is a
 * wrapper for advanced non-contiguous FFTW usage. See FFTW documentation for
 * further details.
 *
 *   http://www.fftw.org/doc/Advanced-Complex-DFTs.html
 *
 * It is currently unknown how the offset of the output buffer affects FFTW
 * memory alignment.
 */
struct fft_hdl *init_fft(int reverse, int m, int istride, int ostride,
			 struct cxvec *in, struct cxvec *out, int ooffset)
{
	int rank = 1;
	int n[] = { m };
	int howmany = istride;
	int idist = 1;
	int odist = 1;
	int *inembed = n;
	int *onembed = n;
	fftwf_complex *obuffer, *ibuffer;

	struct fft_hdl *hdl = (struct fft_hdl *) malloc(sizeof(struct fft_hdl));
	if (!hdl)
		return NULL;

	int direction = FFTW_FORWARD;
	if (reverse)
		direction = FFTW_BACKWARD;

	ibuffer = (fftwf_complex *) in->data;
	obuffer = (fftwf_complex *) out->data + ooffset;
	
	hdl->fft_in = in; 
	hdl->fft_out = out; 
	hdl->fft_plan = fftwf_plan_many_dft(rank, n, howmany,
					    ibuffer, inembed, istride, idist,
					    obuffer, onembed, ostride, odist,
					    direction, FFTW_MEASURE);
	return hdl;
}

void *fft_malloc(size_t size)
{
	return fftwf_malloc(size);
}

/*! \brief Free FFT backend resources 
 */
void free_fft(struct fft_hdl *hdl)
{
	cxvec_free(hdl->fft_in);
	cxvec_free(hdl->fft_out);
	fftwf_destroy_plan(hdl->fft_plan);
	free(hdl);
}

/*! \brief Run multiple DFT operations with the initialized plan
 *  \param[in] hdl handle to an intitialized fft struct
 *
 * Input and output buffers are configured with init_fft().
 */
int cxvec_fft(struct fft_hdl *hdl)
{
	fftwf_execute(hdl->fft_plan);
	return 0;
}
