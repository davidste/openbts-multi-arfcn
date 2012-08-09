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
void ChannelizerBase::resetPartitions()
{
	int i;

	for (i = 0; i < mChanM; i++) {
		cxvec_reset(partInputs[i]);
		cxvec_reset(partOutputs[i]);
	}

	cxvec_reset(fftBuffer);
}

/* 
 * Create polyphase filterbank
 *
 * Implementation based material found in, 
 *
 * "harris, fred, Multirate Signal Processing, Upper Saddle River, NJ,
 *     Prentice Hall, 2006."
 */
bool ChannelizerBase::initFilters(struct cxvec **protoFilter)
{
	int i, n;
	int m = mChanM;
	int protoFilterLen = m * mPartitionLen;

	float *protoFilterBase;
	float sum = 0.0f;
	float scale = 0.0f;
	float midpt = protoFilterLen / 2;

	/* 
	 * Allocate 'M' partition filters and the temporary prototype
	 * filter. Coefficients are real only and must be 16-byte memory
	 * aligned for SSE usage.
	 */
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;

	protoFilterBase = (float *) malloc(protoFilterLen * sizeof(float));
	if (!protoFilterBase)
		return false;

	partitions = (struct cxvec **) malloc(sizeof(struct cxvec *) * m);
	if (!partitions)
		return false;

	for (i = 0; i < m; i++) {
		partitions[i] = cxvec_alloc(mPartitionLen, 0, NULL, flags);
	}

	/* 
	 * Generate the prototype filter with a Blackman-harris window.
	 * Scale coefficients with DC filter gain set to unity divided
	 * by the number of channels.
	 */
	float a0 = 0.35875;
	float a1 = 0.48829;
	float a2 = 0.14128;
	float a3 = 0.01168;

	for (i = 0; i < protoFilterLen; i++) {
		protoFilterBase[i] = cxvec_sinc(((float) i - midpt) / m);
		protoFilterBase[i] *= a0 -
				      a1 * cos(2 * M_PI * i / (protoFilterLen - 1)) +
				      a2 * cos(4 * M_PI * i / (protoFilterLen - 1)) -
				      a3 * cos(6 * M_PI * i / (protoFilterLen - 1));
		sum += protoFilterBase[i];
	}
	scale = mChanM / sum;

	/* 
	 * Populate partition filters and reverse the coefficients per
	 * convolution requirements.
	 */
	for (i = 0; i < mPartitionLen; i++) {
		for (n = 0; n < m; n++) {
			partitions[n]->data[i].real = protoFilterBase[i * m + n] * scale;
			partitions[n]->data[i].imag = 0.0f;
		}
	}

	for (i = 0; i < m; i++) {
		cxvec_rvrs(partitions[i], partitions[i]);
	}

	/*
	 * If requested, return the complex vector prototype filter.
	 */
	if (protoFilter) {
		*protoFilter = cxvec_alloc(protoFilterLen, 0, NULL, flags);
		(*protoFilter)->flags = CXVEC_FLG_REAL_ONLY;

		for (i = 0; i < protoFilterLen; i++) {
			(*protoFilter)->data[i].real = protoFilterBase[i] * scale;
			(*protoFilter)->data[i].imag = 0.0f;
		}
	}

	free(protoFilterBase);

	return true;
}

void ChannelizerBase::releaseFilters()
{
	int i;

	for (i = 0; i < mChanM; i++) {
		cxvec_free(partitions[i]);
	}

	free(partitions);
}

bool ChannelizerBase::activateChan(int num)
{
	return mResampler->activateChan(num);
}

bool ChannelizerBase::deactivateChan(int num)
{
	return mResampler->deactivateChan(num);
}

/* 
 * Setup filterbank internals
 */
bool ChannelizerBase::init(struct cxvec **protoFilterBase)
{
	int i;

	/*
	 * Filterbank coefficients, fft plan, history, and output sample
	 * rate conversion blocks
	 */
	if (!initFilters(protoFilterBase)) {
		LOG(ERR) << "Failed to initialize channelizing filter";
		return false;
	}

	fftBuffer = cxvec_alloc(chunkLen * mChanM, 0, NULL, 0);
	if (!fftBuffer) {
		LOG(ERR) << "Memory allocation error";
		return false;
	}

	fftHandle = init_fft(0, mChanM);
	if (!fftHandle) {
		LOG(ERR) << "Failed to initialize FFT";
	}

	mResampler = new Resampler(mP, mQ, mResampLen, mChanM);
	if (!mResampler->init(NULL)) {
		LOG(ERR) << "Failed to initialize resampling filter";
		return false;
	}

	history = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);
	for (i = 0; i < mChanM; i++) {
		history[i] =  cxvec_alloc(mPartitionLen, 0, NULL, 0);
		cxvec_reset(history[i]);
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
	partInputs = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);
	partOutputs = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);

	if (!partInputs | !partOutputs) {
		LOG(ERR) << "Memory allocation error";
		return false;
	}

	for (i = 0; i < mChanM; i++) {
		partInputs[i] = cxvec_alloc(chunkLen + mPartitionLen,
					    mPartitionLen, NULL, 0);
		partOutputs[i] = cxvec_alloc(chunkLen + mResampLen,
					     mResampLen, NULL, 0);
	}

	return true;
}

/* 
 * Setup channelizer paramaters
 * 
 * Channelizer operates at multiples of the channel rate and not the
 * transceiver rate, which is a multiple of the GSM symbol rate. The
 * channel rate may be higher or lower than the transceiver rate
 * depending on samples-per-symbol and channel bandwidth. 
 */
ChannelizerBase::ChannelizerBase(int wChanM, int wPartitionLen, int wResampLen,
				 int wP, int wQ, int wMul, chanType type) 
	: mChanM(wChanM), mPartitionLen(wPartitionLen), mResampLen(wResampLen),
	  mP(wP), mQ(wQ), mMul(wMul)
{
	if (type == TX_SYNTHESIS)
		chunkLen = mP * mMul;
	else
		chunkLen = mQ * mMul;
}

ChannelizerBase::~ChannelizerBase()
{
	int i;

	releaseFilters();
	cxvec_free(fftBuffer);
	free_fft(fftHandle);

	for (i = 0; i < mChanM; i++) {
		cxvec_free(partInputs[i]);
		cxvec_free(partOutputs[i]);
		cxvec_free(history[i]);
	}

	free(partInputs);
	free(partOutputs);
	free(history);
}
