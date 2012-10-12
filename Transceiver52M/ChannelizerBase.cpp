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

/* 
 * Create polyphase filterbank
 *
 * Implementation based material found in, 
 *
 * "harris, fred, Multirate Signal Processing, Upper Saddle River, NJ,
 *     Prentice Hall, 2006."
 */
bool ChannelizerBase::initFilters()
{
	int i, n;
	int m = mChanM;
	int protoLen = m * mFiltLen;

	float *proto;
	float sum = 0.0f;
	float scale = 0.0f;
	float midpt = protoLen / 2;

	/* 
	 * Allocate 'M' partition filters and the temporary prototype
	 * filter. Coefficients are real only and must be 16-byte memory
	 * aligned for SSE usage.
	 */
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;

	proto = (float *) malloc(protoLen * sizeof(float));
	if (!proto)
		return false;

	subFilters = (struct cxvec **) malloc(sizeof(struct cxvec *) * m);
	if (!subFilters)
		return false;

	for (i = 0; i < m; i++) {
		subFilters[i] = cxvec_alloc(mFiltLen, 0, NULL, flags);
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

	for (i = 0; i < protoLen; i++) {
		proto[i] = cxvec_sinc(((float) i - midpt) / m);
		proto[i] *= a0 -
			    a1 * cos(2 * M_PI * i / (protoLen - 1)) +
			    a2 * cos(4 * M_PI * i / (protoLen - 1)) -
			    a3 * cos(6 * M_PI * i / (protoLen - 1));
		sum += proto[i];
	}
	scale = mChanM / sum;

	/* 
	 * Populate partition filters and reverse the coefficients per
	 * convolution requirements.
	 */
	for (i = 0; i < mFiltLen; i++) {
		for (n = 0; n < m; n++) {
			subFilters[n]->data[i].real = proto[i * m + n] * scale;
			subFilters[n]->data[i].imag = 0.0f;
		}
	}

	for (i = 0; i < m; i++) {
		cxvec_rvrs(subFilters[i], subFilters[i]);
	}

	free(proto);

	return true;
}

void ChannelizerBase::releaseFilters()
{
	for (int i = 0; i < mChanM; i++) {
		cxvec_free(subFilters[i]);
	}

	free(subFilters);
}

bool ChannelizerBase::activateChan(int num)
{
	return mResampler->activateChan(num);
}

bool ChannelizerBase::deactivateChan(int num)
{
	return mResampler->deactivateChan(num);
}

bool ChannelizerBase::initFFT()
{
	int len;
	int flags = CXVEC_FLG_FFT_ALIGN;

	if (fftInput || fftOutput || fftHandle)
		return false;

	len = chunkLen * mChanM;
	fftInput = cxvec_alloc(len, 0, NULL, flags);

	len = (chunkLen + mFiltLen) * mChanM;
	fftOutput = cxvec_alloc(len, 0, NULL, flags);

	if (!fftInput | !fftOutput) {
		LOG(ERR) << "Memory allocation error";
		return false;
	}

	cxvec_reset(fftInput);
	cxvec_reset(fftOutput);

	fftHandle = init_fft(0, mChanM, chunkLen, chunkLen + mFiltLen,
			     fftInput, fftOutput, mFiltLen);

	return true;
}

bool ChannelizerBase::mapBuffers()
{
	int idx;

	if (!fftHandle) {
		LOG(ERR) << "FFT buffers not initialized";
		return false;
	}

	filtInputs = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);
	filtOutputs = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);
	if (!filtInputs | !filtOutputs)
		return false;

	for (int i = 0; i < mChanM; i++) {
		idx = i * (chunkLen + mFiltLen);
		filtInputs[i] =
			cxvec_subvec(fftOutput, idx, mFiltLen, chunkLen);

		idx = i * chunkLen;
		filtOutputs[i] =
			cxvec_subvec(fftInput, idx, 0, chunkLen);
	}
}

/* 
 * Setup filterbank internals
 */
bool ChannelizerBase::init()
{
	int i;

	/*
	 * Filterbank coefficients, fft plan, history, and output sample
	 * rate conversion blocks
	 */
	if (!initFilters()) {
		LOG(ERR) << "Failed to initialize channelizing filter";
		return false;
	}

	mResampler = new Resampler(mP, mQ, mFiltLen, mChanM);
	if (!mResampler->init()) {
		LOG(ERR) << "Failed to initialize resampling filter";
		return false;
	}

	history = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);
	for (i = 0; i < mChanM; i++) {
		history[i] =  cxvec_alloc(mFiltLen, 0, NULL, 0);
		cxvec_reset(history[i]);
	}

	if (!initFFT()) {
		LOG(ERR) << "Failed to initialize FFT";
		return false;
	}

	mapBuffers();

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
ChannelizerBase::ChannelizerBase(int wChanM, int wFiltLen,
				 int wP, int wQ, int wMul, chanType type) 
	: mChanM(wChanM), mFiltLen(wFiltLen), mP(wP), mQ(wQ), mMul(wMul),
	  fftInput(NULL), fftOutput(NULL), fftHandle(NULL)
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
	free_fft(fftHandle);

	for (i = 0; i < mChanM; i++) {
		cxvec_free(filtInputs[i]);
		cxvec_free(filtOutputs[i]);
		cxvec_free(history[i]);
	}

	free(filtInputs);
	free(filtOutputs);
	free(history);
}
