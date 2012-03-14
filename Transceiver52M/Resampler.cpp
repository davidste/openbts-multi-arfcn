/*
 * Rational ratio resampler 
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
#include "Resampler.h"

#define MAX_OUTPUT_LEN		4096

bool Resampler::initFilters(struct cxvec **protoFilter)
{
	int i, n;
	int protoFilterLen = mP * mPartitionLen;

	float *protoFiltBase;
	float sum = 0.0f;
	float scale = 0.0f;
	float midpt = protoFilterLen / 2;

	/* 
	 * Allocate partition filters and the temporary prototype filter
	 * according to numerator of the rational rate. Coefficients are
	 * real only and must be 16-byte memory aligned for SSE usage.
	 */
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;

	protoFiltBase = (float *) malloc(protoFilterLen * sizeof(float));
	if (!protoFiltBase)
		return false;

	partitions = (struct cxvec **) malloc(sizeof(struct cxvec *) * mP);
	if (!partitions)
		return false;

	for (i = 0; i < mP; i++)
		partitions[i] = cxvec_alloc(mPartitionLen, 0, NULL, flags);

	/* 
	 * Generate the prototype filter with a boxcar windowed sinc filter.
	 * Scale coefficients with DC filter gain set to unity divided
	 * by the number of filter partitions. 
	 */
	for (i = 0; i < protoFilterLen; i++) {
		protoFiltBase[i] = cxvec_sinc(((float) i - midpt) / mP);
		sum += protoFiltBase[i];
	}
	scale = mP / sum;

	/* 
	 * Populate partition filters and reverse the coefficients per
	 * convolution requirements.
	 */
	for (i = 0; i < mPartitionLen; i++) {
		for (n = 0; n < mP; n++) {
			partitions[n]->data[i].real = protoFiltBase[i * mP + n] * scale;
			partitions[n]->data[i].imag = 0.0f;
		}
	}

	for (i = 0; i < mP; i++) {
		cxvec_rvrs_conj(partitions[i], partitions[i]);
	}

	/*
	 * If requested, return the complex vector prototype filter.
	 */
	if (protoFilter) {
		*protoFilter = cxvec_alloc(protoFilterLen, 0, NULL, flags);
		(*protoFilter)->flags = CXVEC_FLG_REAL_ONLY;

		for (i = 0; i < protoFilterLen; i++) {
			(*protoFilter)->data[i].real = protoFiltBase[i] * scale;
			(*protoFilter)->data[i].imag = 0.0f; 
		}
	}

	free(protoFiltBase);

	return true;
}

void Resampler::releaseFilters()
{
	int i;

	for (i = 0; i < mQ; i++) {
		cxvec_free(partitions[i]);
	}

	free(partitions);
}

static bool check_vec_len(struct cxvec *in,
			 struct cxvec *out,
			 int p, int q)
{
	if (in->len % q) {
		LOG(ERR) << "Invalid input length " << in->len
			 <<  " is not multiple of " << q;
		return false;
	}

	if (out->len % p) {
		LOG(ERR) << "Invalid output length " << out->len
			 <<  " is not multiple of " << p;
		return false;
	}

	if ((in->len / q) != (out->len / p)) {
		LOG(ERR) << "Input/output block length mismatch";
		return false;
	}

	if (out->len > MAX_OUTPUT_LEN) {
		LOG(ERR) << "Block length of " << out->len << " exceeds max";
		return false;
	}

	return true;
}

void Resampler::computePath()
{
	int i;

	for (i = 0; i < MAX_OUTPUT_LEN; i++) {
		inputIndex[i] = (mQ * i) / mP;
		outputPath[i] = (mQ * i) % mP;
	}
}

int Resampler::rotateSingle(struct cxvec *in, struct cxvec *out, struct cxvec *hist)
{
	int i, n, path;

	if (!check_vec_len(in, out, mP, mQ))
		return -1; 

	/* Insert history */
	memcpy(in->buf, hist->data, hist->len * sizeof(cmplx));

	/* Generate output from precomputed input/output paths */
	for (i = 0; i < out->len; i++) {
		n = inputIndex[i]; 
		path = outputPath[i]; 

		single_convolve(&in->data[n],
				partitions[path],
				&out->data[i]);
	}

	/* Save history */
	memcpy(hist->data, &in->data[in->len - hist->len],
	       hist->len * sizeof(cmplx));

	return out->len;
}

int Resampler::rotate(struct cxvec **in, struct cxvec **out)
{
	int i, len;

	for (i = 0; i < mChanM; i++) {
		if (chanActive[i]) {
			len = rotateSingle(in[i], out[i], history[i]);
		}
	}

	return len;
}

bool Resampler::activateChan(int num)
{
	if ((num >= mChanM) || (num < 0)) {
		LOG(ERR) << "Invalid channel selection";
		return false;
	}

	if (chanActive[num]) {
		LOG(ERR) << "Channel already activated";
		return false;
	}

	chanActive[num] = true;

	return true;
}

bool Resampler::deactivateChan(int num)
{
	if ((num >= mChanM) || (num < 0)) {
		LOG(ERR) << "Invalid channel selection";
		return false;
	}

	if (chanActive[num]) {
		LOG(ERR) << "Channel already activated";
		return false;
	}

	chanActive[num] = true;

	return true;
}

bool Resampler::init(struct cxvec **protoFilter)
{
	int i, rc;

	/* Filterbank internals */
	rc = initFilters(protoFilter);
	if (rc < 0) {
		LOG(ERR) << "Failed to create resampler filterbank";
		return false;
	}

	/* History buffer */
	history = (struct cxvec **) malloc(sizeof(struct cxvec *) * mChanM);
	if (!history) {
		LOG(ERR) << "Memory allocation failed";
		return false;
	}

	for (i = 0; i < mChanM; i++) {
		history[i] = cxvec_alloc(mPartitionLen, 0, NULL, 0);
		cxvec_reset(history[i]);
	}

	/* Channel activation status */
	chanActive = (bool *) malloc(sizeof(bool) * mChanM);
	if (!chanActive) {
		LOG(ERR) << "Memory allocation failed";
		return false;
	}

	for (i = 0; i < mChanM; i++) {
		chanActive[i] = false;
	}

	/* Precompute filterbank path */
	inputIndex = (int *) malloc(sizeof(int) * MAX_OUTPUT_LEN);
	outputPath = (int *) malloc(sizeof(int) * MAX_OUTPUT_LEN);
	computePath();

	return true;
}

Resampler::Resampler(int wP, int wQ, int wPartitionLen, int wChanM)
	: mP(wP), mQ(wQ),
	  mPartitionLen(wPartitionLen), mChanM(wChanM),
	  inputIndex(NULL), outputPath(NULL)
{
}

Resampler::~Resampler()
{
	int i;

	releaseFilters();

	for (i = 0; i < mChanM; i++) {
		cxvec_free(history[i]);
	}

	free(inputIndex);
	free(outputPath);
	free(history);
	free(chanActive);
}
