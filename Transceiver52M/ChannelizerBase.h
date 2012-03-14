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
	int mP;
	int mQ;
	int mMul;
	int chunkLen; 

	/* Channelizer parameters */
	int mChanM;
	int mPartitionLen;

	/* Channelizer internal filterbank buffers */
	struct cxvec **partitions;
	struct cxvec **partInputs;
	struct cxvec **partOutputs;
	struct cxvec **history;
	struct cxvec *fftBuffer;

	/* Output sample rate converter */
	Resampler *mResampler;
	int mResampLen;

	/* Initializer internals */
	bool initFilters(struct cxvec **protoFilter);
	void releaseFilters();
	void resetPartitions();

	/* Direction */
	enum chanType {
		RX_CHANNELIZER,
		TX_SYNTHESIS
	};

	ChannelizerBase(int wChanM, int wPartitionLen, int wResampLen,
			int wP, int wQ, int wMul, chanType type);
	~ChannelizerBase();

public:
	/** Initilize channelizer/synthesis filter internals
	    @param prot_filt optional pointer reference to store prototype filter
	    @return negative value on error, zero otherwise
	 */
	bool init(struct cxvec **protoFilter);

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
