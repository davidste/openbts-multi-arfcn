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

#ifndef _DOWNSAMPLER_H_
#define _DOWNSAMPLER_H_

#include "sigproc/sigproc.h"

class Resampler {
private:
	int filt_rat_num;
	int filt_rat_den;
	int part_filt_len;
	int filt_m;
	bool *chan_active;

	int *input_idx;
	int *output_path;

	struct cxvec **part_filt_vecs;
	struct cxvec **hist_vecs;

	bool init_filt(struct cxvec **fill_prot_filt);
	void precompute_path();
	void release_dnsmpl_filt();
	int rotate_single(struct cxvec *in, struct cxvec *out, struct cxvec *hist);

public:
	/** Constructor for 'M' channel rational resampler 
	    @param r_num numerator of resampling ratio
	    @param r_den denominator of resampling ratio
	    @param filt_len length of each partition filters
	    @param m number of channels
	*/
	Resampler(int r_num, int r_den, int filt_len, int m);
	~Resampler();

	/** Initilize resampler filterbank 
	    @param prot_filt optional pointer reference to store prototype filter
	    @return negative value on error, zero otherwise
	 */
	bool init(struct cxvec **prot_filt);

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

#endif /* _DOWNSAMPLER_H_ */

