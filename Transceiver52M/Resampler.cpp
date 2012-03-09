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

bool Resampler::init_filt(struct cxvec **fill_prot_filt)
{
	int i, n;
	int m = filt_rat_num;
	int prot_filt_len = m * part_filt_len;

	float *prot_filt;
	float sum = 0.0f;
	float scale = 0.0f;
	float midpt = prot_filt_len / 2;

	/* 
	 * Allocate partition filters and the temporary prototype filter
	 * according to numerator of the rational rate. Coefficients are
	 * real only and must be 16-byte memory aligned for SSE usage.
	 */
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;

	prot_filt = (float *) malloc(prot_filt_len * sizeof(float));
	if (!prot_filt)
		return false;

	part_filt_vecs = (struct cxvec **) malloc(sizeof(struct cxvec *) * m);
	if (!part_filt_vecs)
		return false;

	for (i = 0; i < m; i++) {
		part_filt_vecs[i] = cxvec_alloc(part_filt_len, 0, NULL, flags);
		part_filt_vecs[i]->flags |= CXVEC_FLG_REAL_ONLY;
	}

	/* 
	 * Generate the prototype filter with a boxcar windowed sinc filter.
	 * Scale coefficients with DC filter gain set to unity divided
	 * by the number of filter partitions. 
	 */
	for (i = 0; i < prot_filt_len; i++) {
		prot_filt[i] = cxvec_sinc(((float) i - midpt) / filt_rat_num);
		sum += prot_filt[i];
	}
	scale = filt_rat_num / sum;

	/* 
	 * Populate partition filters and reverse the coefficients per
	 * convolution requirements.
	 */
	for (i = 0; i < part_filt_len; i++) {
		for (n = 0; n < m; n++) {
			part_filt_vecs[n]->data[i].real = prot_filt[i * m + n] * scale;
			part_filt_vecs[n]->data[i].imag = 0.0f;
		}
	}

	for (i = 0; i < m; i++) {
		cxvec_rvrs_conj(part_filt_vecs[i], part_filt_vecs[i]);
	}

	/*
	 * If requested, return the complex vector prototype filter.
	 */
	if (fill_prot_filt) {
		*fill_prot_filt = cxvec_alloc(prot_filt_len, 0, NULL, flags);
		(*fill_prot_filt)->flags = CXVEC_FLG_REAL_ONLY;

		for (i = 0; i < prot_filt_len; i++) {
			(*fill_prot_filt)->data[i].real = prot_filt[i] * scale;
			(*fill_prot_filt)->data[i].imag = 0.0f; 
		}
	}

	free(prot_filt);

	return true;
}

void Resampler::release_dnsmpl_filt()
{
	int i;

	for (i = 0; i < filt_rat_den; i++) {
		cxvec_free(part_filt_vecs[i]);
	}

	free(part_filt_vecs);
}

static bool check_vec_len(struct cxvec *in,
			 struct cxvec *out,
			 int rat_num, int rat_den)
{
	if (in->len % rat_den) {
		LOG(ERR) << "Invalid input length " << in->len
			 <<  " is not multiple of " << rat_den;
		return false;
	}

	if (out->len % rat_num) {
		LOG(ERR) << "Invalid output length " << out->len
			 <<  " is not multiple of " << rat_num;
		return false;
	}

	if ((in->len / rat_den) != (out->len / rat_num)) {
		LOG(ERR) << "Input/output block length mismatch";
		return false;
	}

	if (out->len > MAX_OUTPUT_LEN) {
		LOG(ERR) << "Block length of " << out->len << " exceeds max";
		return false;
	}

	return true;
}

void Resampler::precompute_path()
{
	int i;

	for (i = 0; i < MAX_OUTPUT_LEN; i++) {
		input_idx[i]  = filt_rat_den * i / filt_rat_num;
		output_path[i] = (filt_rat_den * i) % filt_rat_num;
	}
}

int Resampler::rotate_single(struct cxvec *in,
			     struct cxvec *out, struct cxvec *hist)
{
	int i, n, path;

	if (!check_vec_len(in, out, filt_rat_num, filt_rat_den))
		return -1; 

	/* Insert history */
	memcpy(in->buf, hist->data, hist->len * sizeof(cmplx));

	/* Generate output from precomputed input/output paths */
	for (i = 0; i < out->len; i++) {
		n = input_idx[i]; 
		path = output_path[i]; 

		single_convolve(&in->data[n],
				part_filt_vecs[path],
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

	for (i = 0; i < filt_m; i++) {
		if (chan_active[i]) {
			len = rotate_single(in[i], out[i], hist_vecs[i]);
		}
	}

	return len;
}

bool Resampler::activateChan(int num)
{
	if ((num >= filt_m) || (num < 0)) {
		LOG(ERR) << "Invalid channel selection";
		return false;
	}

	if (chan_active[num]) {
		LOG(ERR) << "Channel already activated";
		return false;
	}

	chan_active[num] = true;

	return true;
}

bool Resampler::deactivateChan(int num)
{
	if ((num >= filt_m) || (num < 0)) {
		LOG(ERR) << "Invalid channel selection";
		return false;
	}

	if (chan_active[num]) {
		LOG(ERR) << "Channel already activated";
		return false;
	}

	chan_active[num] = true;

	return true;
}

bool Resampler::init(struct cxvec **prot_filt)
{
	int i, rc;

	/* Filterbank internals */
	rc = init_filt(prot_filt);
	if (rc < 0) {
		LOG(ERR) << "Failed to create resampler filterbank";
		return false;
	}

	/* History buffer */
	hist_vecs = (struct cxvec **) malloc(sizeof(struct cxvec *) * filt_m);
	if (!hist_vecs) {
		LOG(ERR) << "Memory allocation failed";
		return false;
	}

	for (i = 0; i < filt_m; i++) {
		hist_vecs[i] = cxvec_alloc(part_filt_len, 0, NULL, 0);
		cxvec_reset(hist_vecs[i]);
	}

	/* Channel activation status */
	chan_active = (bool *) malloc(sizeof(bool) * filt_m);
	if (!chan_active) {
		LOG(ERR) << "Memory allocation failed";
		return false;
	}

	for (i = 0; i < filt_m; i++) {
		chan_active[i] = false;
	}

	/* Precompute filterbank path */
	input_idx = (int *) malloc(sizeof(int) * MAX_OUTPUT_LEN);
	output_path = (int *) malloc(sizeof(int) * MAX_OUTPUT_LEN);
	precompute_path();

	return true;
}

Resampler::Resampler(int r_num, int r_den, int filt_len, int chans)
	: filt_rat_num(r_num), filt_rat_den(r_den),
	  part_filt_len(filt_len), filt_m(chans),
	  input_idx(NULL), output_path(NULL)
{
}

Resampler::~Resampler()
{
	int i;

	release_dnsmpl_filt();

	for (i = 0; i < filt_m; i++) {
		cxvec_free(hist_vecs[i]);
	}

	free(input_idx);
	free(output_path);
	free(hist_vecs);
	free(chan_active);
}
