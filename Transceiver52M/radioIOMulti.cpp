/*
 * Channelizing radio interface
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

#include <Synthesis.h>
#include <Channelizer.h>
#include <radioInterface.h>
#include <Logger.h>

#include "sigproc/sigproc.h"

/* New chunk sizes for resampled rate */
#ifdef INCHUNK
  #undef INCHUNK
#endif
#ifdef OUTCHUNK
  #undef OUTCHUNK
#endif

/* Channelizer parameters */
#define CHUNKMUL		9

#define INRATE			(2 * 65 * SAMPSPERSYM)
#define INCHUNK			(INRATE * CHUNKMUL)
#define INBUFLEN		(INCHUNK * 4)

#define OUTRATE			(96 * SAMPSPERSYM)
#define OUTCHUNK		(OUTRATE * CHUNKMUL * CHAN_M)
#define OUTBUFLEN		(OUTCHUNK * 4)

#define CHAN_FILT_LEN		16
#define RESAMP_FILT_LEN		8

static struct cxvec *hr_tx_vec;
static struct cxvec *hr_rx_vec;

static struct cxvec *lr_tx_vecs[CHAN_M];
static struct cxvec *lr_rx_vecs[CHAN_M];

static Channelizer *chan;
static Synthesis *synth;

/* Initialize I/O specific objects */
bool RadioInterface::init()
{
	int i;

	chan = new Channelizer(CHAN_M, CHAN_FILT_LEN, RESAMP_FILT_LEN,
			       INRATE, OUTRATE, CHUNKMUL);
	if (!chan->init(NULL)) {
		LOG(ALERT) << "Rx channelizer failed to initialize";
		return false;
	}

	synth = new Synthesis(CHAN_M, CHAN_FILT_LEN, RESAMP_FILT_LEN,
			      OUTRATE, INRATE, CHUNKMUL);
	if (!synth->init(NULL)) {
		LOG(ALERT) << "Tx channelizer failed to initialize";
		return false;
	}

	hr_tx_vec = cxvec_alloc(OUTBUFLEN, 0, NULL, 0);
	hr_rx_vec = cxvec_alloc(OUTBUFLEN, 0, NULL, 0);

	/*
	 * Setup per-channel variables. The low rate transmit vectors 
	 * feed into the resampler prior to the synthesis filter
	 * and requires headroom equivalent to the filter length. Low
	 * rate buffers are allocated in the main radio interface code.
	 */
	for (i = 0; i < CHAN_M; i++) {
		if (chanActive[i]) {
			chan->activateChan(i);
			synth->activateChan(i);
		}

		lr_rx_vecs[i] =
			cxvec_alloc(2 * 625, 0, (cmplx *) rcvBuffer[i], 0);
		lr_tx_vecs[i] =
			cxvec_alloc(2 * 625, RESAMP_FILT_LEN, (cmplx *) sendBuffer[i], 0);
	}

	return true;
}

void RadioInterface::close()
{
	int i;

	cxvec_free(hr_tx_vec);
	cxvec_free(hr_rx_vec);

	/* Don't deallocate class member buffers */
	for (i = 0; i < CHAN_M; i ++) {
		lr_rx_vecs[i]->buf = NULL;
		lr_tx_vecs[i]->buf = NULL;

		cxvec_free(lr_rx_vecs[i]);
		cxvec_free(lr_tx_vecs[i]);
	}

	delete chan;
	delete synth;
}

/* Receive a timestamped chunk from the device */
void RadioInterface::pullBuffer()
{
	int i, num_cv, num_rd;
	bool local_underrun;

	/* Read samples. Fail if we don't get what we want. */
	hr_rx_vec->len = mRadio->readSamples((float *) hr_rx_vec->data,
					     OUTCHUNK, &overrun, readTimestamp,
					     &local_underrun);

	LOG(DEBUG) << "Rx read " << hr_rx_vec->len << " samples from device";
	assert(hr_rx_vec->len == OUTCHUNK);

	underrun |= local_underrun;
	readTimestamp += (TIMESTAMP) hr_rx_vec->len;

	for (i = 0; i < CHAN_M; i++) {
		lr_rx_vecs[i]->start_idx = rcvCursor;
		lr_rx_vecs[i]->data = &lr_rx_vecs[i]->buf[rcvCursor];
		lr_rx_vecs[i]->len = INCHUNK;
	}

	/* Channelize */
	num_cv = chan->rotate(hr_rx_vec, lr_rx_vecs);

	rcvCursor += num_cv;
}

/* Resize data buffers of length 'len' after reading 'n' samples */
static void shift_tx_bufs(struct cxvec **vecs, int len, int n)
{
	int i;

	for (i = 0; i < CHAN_M; i++) {
		memmove(vecs[i]->data, &vecs[i]->data[n],
			(len - n) * sizeof(cmplx));
	}
}

/* Send a timestamped chunk to the device */
void RadioInterface::pushBuffer()
{
	int i, num_cv, num_chunks;

	if (sendCursor < INCHUNK)
		return;

	/* We only handle 1 */
	num_chunks = 1;

	for (i = 0; i < CHAN_M; i++) {
		lr_tx_vecs[i]->len = num_chunks * INCHUNK;
	}

	hr_tx_vec->len = num_chunks * OUTCHUNK;

	num_cv = synth->rotate(lr_tx_vecs, hr_tx_vec);

	/* Write samples. Fail if we don't get what we want. */
	int num_smpls = mRadio->writeSamples((float *) hr_tx_vec->data,
					     num_cv,
					     &underrun,
					     writeTimestamp);
	assert(num_smpls == num_cv);

	writeTimestamp += (TIMESTAMP) num_smpls;

	/* Move unsent samples to beginning of buffer */
	shift_tx_bufs(lr_tx_vecs, sendCursor, lr_tx_vecs[0]->len);
	sendCursor -= lr_tx_vecs[0]->len;

	assert(sendCursor >= 0);
}

bool RadioInterface::activateChan(int num)
{
	if ((num >= CHAN_M) || (num < 0)) {
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

bool RadioInterface::deactivateChan(int num)
{
	if ((num >= CHAN_M) || (num < 0)) {
		LOG(ERR) << "Invalid channel selection";
		return false;
	}

	if (chanActive[num]) {
		LOG(ERR) << "Channel not active";
		return false;
	}

	chanActive[num] = false;

	return true;
}
