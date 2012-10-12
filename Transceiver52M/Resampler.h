/*
 * Rational ratio Resampler 
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

#ifndef _RESAMPLER_H_
#define _RESAMPLER_H_

#include "sigproc/sigproc.h"

class Resampler {
private:
	int mP;
	int mQ;
	int mChanM;
	int mFiltLen;

	bool *chanActive;
	int *inputIndex;
	int *outputPath;

	struct cxvec **partitions;
	struct cxvec **history;

	bool initFilters();
	void releaseFilters();
	void computePath();

	int rotateSingle(struct cxvec *in, struct cxvec *out, struct cxvec *hist);

public:
	/** Constructor for 'M' channel rational resampler 
	    @param wP numerator of resampling ratio
	    @param wQ denominator of resampling ratio
	    @param wFiltLen length of each polyphase subfilter 
	    @param wM number of channels
	*/
	Resampler(int wP, int wQ, int wFiltLen, int wChanM);
	~Resampler();

	/** Initilize resampler filterbank 
	    @return negative value on error, zero otherwise
	 */
	bool init();

	/** Rotate "commutator" and drive samples through filterbank
	    @param in_vecs set of 'M' input vectors 
	    @param out_vecs set of 'M' output vectors
	    @return number of samples outputted per each channel

	    Input and output vector lengths must of be equal multiples of the
	    rational conversion rate denominator and numerator respectively.
	 */
	int rotate(struct cxvec **in, struct cxvec **out);

	/** Activate a specific channel of the resampler
	    @param num channel number to activate
	    @return false on failure, true otherwise
	 */
	bool activateChan(int num);

	/** Deactivate a specific channel of the resampler
	    @param num channel number to deactivate
	    @return false on failure, true otherwise
	 */
	bool deactivateChan(int num);
};

#endif /* _RESAMPLER_H_ */
