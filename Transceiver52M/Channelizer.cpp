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
#include "Channelizer.h"

/* Check vector length validity */
static bool check_vec_len(struct cxvec *in_vec, struct cxvec **out_vecs,
			 int rat_num, int rat_den, int rat_mul)
{
	if (in_vec->len % (rat_den * rat_mul)) {
		LOG(ERR) << "Invalid input length " << in_vec->len
			 <<  " is not multiple of " << rat_den * rat_mul;
		return false;
	}

	if (out_vecs[0]->len % (rat_num * rat_mul)) {
		LOG(ERR) << "Invalid output length " << out_vecs[0]->len
			 <<  " is not multiple of " << rat_num * rat_mul;
		return false;
	}

	return true;
}

/* 
 * Implementation based on material found in:
 *
 * "harris, fred, Multirate Signal Processing, Upper Saddle River, NJ,
 *     Prentice Hall, 2006."
 */
int Channelizer::rotate(struct cxvec *in_vec, struct cxvec **out_vecs)
{
	int i, len;

	if (!check_vec_len(in_vec, out_vecs, resmpl_rat_num,
			   resmpl_rat_den, resmpl_rat_mul)) {
		return -1;
	}

	/* 
	 * Reset and load parition vectors from input vector
	 */
	reset_chan_parts();
	cxvec_deinterlv_rv(in_vec, part_in_vecs, chan_m);

	/* 
	 * Convolve through filterbank while applying and saving sample history 
	 */
	for (i = 0; i < chan_m; i++) {	
		memcpy(part_in_vecs[i]->buf, hist_vecs[i]->data,
		       chan_filt_len * sizeof(cmplx));

		cxvec_convolve(part_in_vecs[i], chan_filt_vecs[i], part_out_vecs[i]);

		memcpy(hist_vecs[i]->data,
		       &part_in_vecs[i]->data[part_in_vecs[i]->len - chan_filt_len],
		       chan_filt_len * sizeof(cmplx));
	}

	/* 
	 * Interleave convolution output into FFT
	 * Deinterleave back into partition output buffers
	 */
	cxvec_interlv(part_out_vecs, fft_vec, chan_m);
	cxvec_fft(fft_vec, fft_vec);
	cxvec_deinterlv_fw(fft_vec, part_out_vecs, chan_m);

	/* 
	 * Downsample FFT output from channel rate multiple to GSM symbol rate
	 */
	len = resmplr->rotate(part_out_vecs, out_vecs);

	return len;
}

/* Setup channelizer paramaters */
Channelizer::Channelizer(int m, int chan_len, int resmpl_len,
			 int r_num, int r_den, int r_mul) 
	: ChannelizerBase(m, chan_len, resmpl_len,
			  r_num, r_den, r_mul, RX_CHANNELIZER)
{
}

Channelizer::~Channelizer()
{
}
