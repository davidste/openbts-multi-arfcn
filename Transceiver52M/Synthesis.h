/*
 * Polyphase synthesis filter 
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

#ifndef _SYNTHESIS_H_
#define _SYNTHESIS_H_

#include "ChannelizerBase.h"

class Synthesis : public ChannelizerBase {
public:
	/** Constructor for synthesis filterbank
	    @param m number of channels
	    @param chan_len length of each channelizer partition filter
	    @param resmpl_len length of each resampler partition filter
	    @param r_num numerator of resampling ratio
	    @param r_den denominator of resampling ratio
	    @param r_mul ratio multiplification factor
	*/
	Synthesis(int m, int chan_len, int resmpl_len,
		  int r_num, int r_den, int r_mul);
	~Synthesis();

	/** Rotate "output commutator" and drive samples through filterbank
	    @param in_vecs set of 'M' input vectors 
	    @param out_vec output vector 
	    @return number of samples outputted
	 */
	int rotate(struct cxvec **in_vecs, struct cxvec *out_vec);
};

#endif /* _CHANNELIZER_TX_H_ */
