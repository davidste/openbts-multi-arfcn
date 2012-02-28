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

#ifndef _CHANNELIZER_RX_H_
#define _CHANNELIZER_RX_H_

#include "ChannelizerBase.h"

class Channelizer : public ChannelizerBase {
public:
	/** Constructor for channelizing filter bank
	    @param m number of channels
	    @param chan_len length of each channelizer partition filter
	    @param resmpl_len length of each resampler partition filter
	    @param r_num numerator of resampling ratio
	    @param r_den denominator of resampling ratio
	    @param r_mul ratio multiplification factor
	*/
	Channelizer(int m, int chan_len, int resmpl_len,
		    int r_num, int r_den, int r_mul);
	~Channelizer();

	/** Rotate "input commutator" and drive samples through filterbank
	    @param in_vec input vector 
	    @param out_vecs set of 'M' output vectors 
	    @return number of samples outputted for each output vector
	 */
	int rotate(struct cxvec *in_vec, struct cxvec **out_vecs);
};

#endif /* _CHANNELIZER_RX_H_ */
