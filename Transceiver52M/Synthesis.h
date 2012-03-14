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
	    @param wChanM number of channels
	    @param wPartitionLen length of each channelizer partition filter
	    @param wResampLen length of each resampler partition filter
	    @param wP numerator of resampling ratio
	    @param wQ denominator of resampling ratio
	    @param mMul ratio multiplification factor
	*/
	Synthesis(int wChanM, int wPartitionLen, int wResampLen,
		  int wP, int wQ, int mMul);
	~Synthesis();

	/** Rotate "output commutator" and drive samples through filterbank
	    @param in set of 'M' input vectors 
	    @param out output vector 
	    @return number of samples outputted
	 */
	int rotate(struct cxvec **in, struct cxvec *out);
};

#endif /* _SYNTHESIS_H_ */
