/*
 * Polyphase channelizer
 * 
 * Copyright 2012  Thomas Tsou <ttsou@vt.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * See the COPYING file in the main directory for details.
 */

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <cstdio>

#include "Logger.h"
#include "ChannelizerBase.h"
#include "sigproc/sigproc.h"

/* Reset filterbank input/output partition vectors */
void ChannelizerBase::reset_chan_parts()
{
	int i;

	for (i = 0; i < chan_m; i++) {
		cxvec_reset(part_in_vecs[i]);
		cxvec_reset(part_out_vecs[i]);
	}

	cxvec_reset(fft_vec);
}

/* 
 * Create polyphase filterbank
 *
 * Implementation based material found in, 
 *
 * "harris, fred, Multirate Signal Processing, Upper Saddle River, NJ,
 *     Prentice Hall, 2006."
 */
bool ChannelizerBase::init_chan_filt(struct cxvec **fill_prot_filt)
{
	int i, n;
	int m = chan_m;
	int prot_filt_len = chan_m * chan_filt_len;

	float *prot_filt;
	float sum = 0.0f;
	float scale = 0.0f;
	float midpt = prot_filt_len / 2;

	/* 
	 * Allocate 'M' partition filters and the temporary prototype
	 * filter. Coefficients are real only and must be 16-byte memory
	 * aligned on 64-bit systems for SSE usage.
	 *
	 * FIXME check memory alignment requirements for non-64-bit systems.
	 */
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;

	prot_filt = (float *) malloc(prot_filt_len * sizeof(float));
	if (!prot_filt)
		return false;

	chan_filt_vecs = (struct cxvec **) malloc(sizeof(struct cxvec *) * m);
	if (!chan_filt_vecs)
		return false;

	for (i = 0; i < m; i++) {
		chan_filt_vecs[i] = cxvec_alloc(chan_filt_len, 0, NULL, flags);
	}

	/* 
	 * Generate the prototype filter with a boxcar windowed sinc filter.
	 * Scale coefficients with DC filter gain set to unity divided
	 * by the number of channels.
	 */
	for (i = 0; i < prot_filt_len; i++) {
		prot_filt[i] = cxvec_sinc(((float) i - midpt) / chan_m);
		sum += prot_filt[i];
	}
	scale = chan_m / sum;

	/* 
	 * Populate partition filters and reverse the coefficients per
	 * convolution requirements.
	 */
	for (i = 0; i < chan_filt_len; i++) {
		for (n = 0; n < m; n++) {
			chan_filt_vecs[n]->data[i].real = prot_filt[i * m + n] * scale;
			chan_filt_vecs[n]->data[i].imag = 0.0f;
		}
	}

	for (i = 0; i < m; i++) {
		cxvec_rvrs_conj(chan_filt_vecs[i], chan_filt_vecs[i]);
	}

	/*
	 * If requested, return the complex vector prototype filter.
	 */
	if (fill_prot_filt) {
		*fill_prot_filt = cxvec_alloc(prot_filt_len, 0, NULL, flags);
		(*fill_prot_filt)->flags = CXVEC_FLG_REAL_ONLY;

		for (i = 0; i < prot_filt_len; i++) {
			(*fill_prot_filt)->data[i].real = prot_filt[i] * scale;
			(*fill_prot_filt)->data[i].imag = 0.0f;
		}
	}

	free(prot_filt);

	return true;
}

void ChannelizerBase::release_chan_filt()
{
	int i;

	for (i = 0; i < chan_m; i++) {
		cxvec_free(chan_filt_vecs[i]);
	}

	free(chan_filt_vecs);
}

bool ChannelizerBase::activateChan(int num)
{
	return resmplr->activateChan(num);
}

bool ChannelizerBase::deactivateChan(int num)
{
	return resmplr->deactivateChan(num);
}

/* 
 * Setup filterbank internals
 */
bool ChannelizerBase::init(struct cxvec **prot_filt)
{
	int i, rc;

	/*
	 * Filterbank coefficients, fft plan, history, and output sample
	 * rate conversion blocks
	 */
	if (!init_chan_filt(prot_filt)) {
		LOG(ERR) << "Failed to initialize channelizing filter";
		return false;
	}

	fft_vec = cxvec_alloc(chan_chnk * chan_m, 0, NULL, 0);
	if (!fft_vec) {
		LOG(ERR) << "Memory allocation error";
		return false;
	}

	rc = init_fft(0, chan_m);
	if (rc < 0) {
		LOG(ERR) << "Failed to initialize FFT";
		return false;
	}

	resmplr = new Resampler(resmpl_rat_num, resmpl_rat_den,
				resmpl_filt_len, chan_m);
	if (!resmplr->init(NULL)) {
		LOG(ERR) << "Failed to initialize resampling filter";
		return false;
	}

	hist_vecs = (struct cxvec **) malloc(sizeof(struct cxvec *) * chan_m);
	for (i = 0; i < chan_m; i++) {
		hist_vecs[i] =  cxvec_alloc(chan_filt_len, 0, NULL, 0);
		cxvec_reset(hist_vecs[i]);
	}

	/* 
	 * Filterbank partition filter buffers
	 * 
	 * Input partition feeds into convolution and requires history
	 * spanning the tap width - 1. We just use the full tap width for
	 * convenience.
	 *
	 * Output partition feeds into downsampler and convolves from index
	 * zero at tap zero with an output length equal to a high rate chunk.
	 */
	part_in_vecs =
		(struct cxvec **) malloc(sizeof(struct cxvec *) * chan_m);
	part_out_vecs =
		(struct cxvec **) malloc(sizeof(struct cxvec *) * chan_m);

	if (!part_in_vecs | !part_out_vecs) {
		LOG(ERR) << "Memory allocation error";
		return false;
	}

	for (i = 0; i < chan_m; i++) {
		part_in_vecs[i] = cxvec_alloc(chan_chnk + chan_filt_len,
					      chan_filt_len, NULL, 0);
		part_out_vecs[i] = cxvec_alloc(chan_chnk + resmpl_filt_len,
					       resmpl_filt_len, NULL, 0);
	}

	return true;
}

/* Setup channelizer paramaters */
ChannelizerBase::ChannelizerBase(int m, int chan_len, int resamp_len,
				 int r_num, int r_den, int r_mul) 
	: chan_m(m), chan_filt_len(chan_len), resmpl_filt_len(resamp_len),
	  resmpl_rat_num(r_num), resmpl_rat_den(r_den), resmpl_rat_mul(r_mul)
{
	/* Channelizer internally operates at multiples of the high rate */
	if (r_num < r_den)
		chan_chnk = r_num * r_mul;
	else
		chan_chnk = r_den * r_mul;
}

ChannelizerBase::~ChannelizerBase()
{
	int i;

	release_chan_filt();
	cxvec_free(fft_vec);
	free_fft();

	for (i = 0; i < chan_m; i++) {
		cxvec_free(part_in_vecs[i]);
		cxvec_free(part_out_vecs[i]);
		cxvec_free(hist_vecs[i]);
	}

	free(part_in_vecs);
	free(part_out_vecs);
	free(hist_vecs);
}
