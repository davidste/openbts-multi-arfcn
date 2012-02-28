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

#ifndef _CHANNELIZER_BASE_H_
#define _CHANNELIZER_BASE_H_

#include "Resampler.h"

class ChannelizerBase {
protected:
	/* Sample rate conversion factors */
	int resmpl_rat_num;
	int resmpl_rat_den;
	int resmpl_rat_mul;
	int chan_chnk;
	int dev_rt;

	/* Channelizer parameters */
	int chan_m;
	int chan_filt_len;

	/* Channelizer internal filterbanks buffers */
	struct cxvec **chan_filt_vecs;
	struct cxvec **part_in_vecs;
	struct cxvec **part_out_vecs;
	struct cxvec **hist_vecs;
	struct cxvec *fft_vec;

	/* Output sample rate converter */
	Resampler *resmplr;
	int resmpl_filt_len;

	/* Initializer internals */
	bool init_chan_filt(struct cxvec **fill_prot_filt);
	void release_chan_filt();
	void reset_chan_parts();

	ChannelizerBase(int m, int chan_len, int filt_len,
			int r_num, int r_den, int r_mul);
	~ChannelizerBase();

public:
	/** Initilize channelizer/synthesis filter internals
	    @param prot_filt optional pointer reference to store prototype filter
	    @return negative value on error, zero otherwise
	 */
	bool init(struct cxvec **prot_filt);

	/** Activate a specific channel of the internal resampler
	    @param num channel number to activate
	    @return false on failure, true otherwise
	 */
	bool activateChan(int num);

	/** Deactivate a specific channel of the internal resampler
	    @param num channel number to deactivate
	    @return false on failure, true otherwise
	 */
	bool deactivateChan(int num);
};

#endif /* _CHANNELIZER_BASE_H_ */
