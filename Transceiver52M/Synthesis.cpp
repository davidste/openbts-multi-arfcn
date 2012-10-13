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

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <cstdio>

#include "Logger.h"
#include "Synthesis.h"

/* Check vector length validity */
static bool checkVectorLen(struct cxvec **in, struct cxvec *out,
			   int p, int q, int mul)
{
	if (in[0]->len % (q * mul)) {
		LOG(ERR) << "Invalid input length " << in[0]->len
			 <<  " is not multiple of " << q * mul;
		return false;
	}

	if (out->len % (p * mul)) {
		LOG(ERR) << "Invalid output length " << out->len
			 <<  " is not multiple of " << p * mul;
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
int Synthesis::rotate(struct cxvec **in, struct cxvec *out)
{
	if (!checkVectorLen(in, out, mP, mQ, mMul)) {
		return -1;
	}

	mResampler->rotate(in, filtOutputs);
	cxvec_fft(fftHandle);

	/* 
	 * Convolve through filterbank while applying and saving sample history 
	 */
	for (int i = 0; i < mChanM; i++) {	
		memcpy(filtInputs[i]->buf, history[i]->data,
		       mFiltLen * sizeof(float complex));

		cxvec_convolve(filtInputs[i], subFilters[i], filtOutputs[i]);

		memcpy(history[i]->data,
		       &filtInputs[i]->data[filtInputs[i]->len - mFiltLen],
		       mFiltLen * sizeof(float complex));
	}

	/* 
	 * Interleave into output vector
	 */
	cxvec_interlv(filtOutputs, out, mChanM);

	return out->len;
}

Synthesis::Synthesis(int wChanM, int wFiltLen, int wP, int wQ, int wMul) 
	: ChannelizerBase(wChanM, wFiltLen, wP, wQ, wMul, TX_SYNTHESIS)
{
}

Synthesis::~Synthesis()
{
}
