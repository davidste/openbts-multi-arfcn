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

static struct cxvec *OuterTxBuf;
static struct cxvec *OuterRxBuf;

static struct cxvec *MiddleTxBuf;
static struct cxvec *MiddleRxBuf;

static struct cxvec *InnerTxBufs[CHAN_MAX];
static struct cxvec *InnerRxBufs[CHAN_MAX];

static Resampler *upsampler;
static Resampler *dnsampler;
static Channelizer *chan;
static Synthesis *synth;

/* Initialize I/O specific objects */
bool RadioInterface::init()
{
        dnsampler = new Resampler(DEV_RESAMP_INRATE,
				  DEV_RESAMP_OUTRATE,
				  DEV_RESAMP_FILT_LEN, 1);
        if (!dnsampler->init()) {
                LOG(ALERT) << "Rx resampler failed to initialize";
                return false;
        }
        dnsampler->activateChan(0);

        upsampler = new Resampler(DEV_RESAMP_OUTRATE,
				  DEV_RESAMP_INRATE,
				  DEV_RESAMP_FILT_LEN, 1);
        if (!upsampler->init()) {
                LOG(ALERT) << "Tx resampler failed to initialize";
                return false;
        }
        upsampler->activateChan(0);

	chan = new Channelizer(mChanM, CHAN_FILT_LEN,
			       GSM_RESAMP_INRATE,
			       GSM_RESAMP_OUTRATE, CHAN_MULT);
	if (!chan->init()) {
		LOG(ALERT) << "Rx channelizer failed to initialize";
		return false;
	}

	synth = new Synthesis(mChanM, CHAN_FILT_LEN,
			      GSM_RESAMP_OUTRATE,
			      GSM_RESAMP_INRATE, CHAN_MULT);
	if (!synth->init()) {
		LOG(ALERT) << "Tx channelizer failed to initialize";
		return false;
	}

	OuterTxBuf = cxvec_alloc(RESAMP_OUTCHUNK * mChanM, 0, NULL, 0);
	OuterRxBuf = cxvec_alloc(RESAMP_OUTCHUNK * mChanM + DEV_RESAMP_FILT_LEN,
				 DEV_RESAMP_FILT_LEN, NULL, 0);
	MiddleTxBuf = cxvec_alloc(RESAMP_INCHUNK * mChanM + DEV_RESAMP_FILT_LEN,
				  DEV_RESAMP_FILT_LEN, NULL, 0);
	MiddleRxBuf = cxvec_alloc(RESAMP_INCHUNK * mChanM, 0, NULL, 0);

	assert(CHAN_OUTCHUNK == RESAMP_INCHUNK);

	/*
	 * Setup per-channel variables. The low rate transmit vectors 
	 * feed into the resampler prior to the synthesis filter
	 * and requires headroom equivalent to the filter length. Low
	 * rate buffers are allocated in the main radio interface code.
	 */
	for (int i = 0; i < mChanM; i++) {
		if (chanActive[i]) {
			chan->activateChan(i);
			synth->activateChan(i);
		}

		InnerRxBufs[i] =
			cxvec_alloc(2 * 625, 0, (float complex *) rcvBuffer[i], 0);
		InnerTxBufs[i] =
			cxvec_alloc(2 * 625, GSM_RESAMP_FILT_LEN, (float complex *) sendBuffer[i], 0);
	}

	return true;
}

void RadioInterface::close()
{
	int i;

	cxvec_free(OuterTxBuf);
	cxvec_free(OuterRxBuf);
	cxvec_free(MiddleTxBuf);
	cxvec_free(MiddleRxBuf);

	for (i = 0; i < mChanM; i ++) {
		cxvec_free(InnerRxBufs[i]);
		cxvec_free(InnerTxBufs[i]);
	}

	delete chan;
	delete synth;
}

/* Receive a timestamped chunk from the device */
void RadioInterface::pullBuffer()
{
	int i, numConverted, numRead;
	bool localUnderrun;

	/* Read samples. Fail if we don't get what we want. */
	numRead = mRadio->readSamples((float *) OuterRxBuf->data,
				      RESAMP_OUTCHUNK * mChanM, &overrun,
				      readTimestamp, &localUnderrun);

	LOG(DEBUG) << "Rx read " << numRead << " samples from device";
	assert(numRead == (RESAMP_OUTCHUNK * mChanM));

	OuterRxBuf->len = numRead;
	underrun |= localUnderrun;
	readTimestamp += (TIMESTAMP) OuterRxBuf->len;

	for (i = 0; i < mChanM; i++) {
		InnerRxBufs[i]->start_idx = rcvCursor;
		InnerRxBufs[i]->data = &InnerRxBufs[i]->buf[rcvCursor];
		InnerRxBufs[i]->len = CHAN_INCHUNK;
	}

	/* Channelize */
	numConverted = dnsampler->rotate(&OuterRxBuf, &MiddleRxBuf);
	numConverted = chan->rotate(MiddleRxBuf, InnerRxBufs);
	rcvCursor += numConverted;
}

/* Resize data buffers of length 'len' after reading 'n' samples */
static void shiftTxBufs(struct cxvec **vecs, int chanM, int len, int n)
{
	int i;

	for (i = 0; i < chanM; i++) {
		memmove(vecs[i]->data, &vecs[i]->data[n],
			(len - n) * sizeof(float complex));
	}
}

/* Send a timestamped chunk to the device */
void RadioInterface::pushBuffer()
{
	int i, numConverted, numChunks, numSent;

	if (sendCursor < CHAN_INCHUNK)
		return;

	/* We only handle 1 */
	numChunks = 1;

	for (i = 0; i < mChanM; i++) {
		InnerTxBufs[i]->len = numChunks * CHAN_INCHUNK;
	}

	MiddleTxBuf->len = numChunks * RESAMP_INCHUNK * mChanM;
	OuterTxBuf->len = numChunks * RESAMP_OUTCHUNK * mChanM;
	numConverted = synth->rotate(InnerTxBufs, MiddleTxBuf);
	numConverted = upsampler->rotate(&MiddleTxBuf, &OuterTxBuf);

	/* Write samples. Fail if we don't get what we want. */
	numSent = mRadio->writeSamples((float *) OuterTxBuf->data,
					numConverted,
					&underrun,
					writeTimestamp);
	assert(numSent == numConverted);
	writeTimestamp += (TIMESTAMP) numSent;

	/* Move unsent samples to beginning of buffer */
	shiftTxBufs(InnerTxBufs, mChanM, sendCursor, InnerTxBufs[0]->len);
	sendCursor -= InnerTxBufs[0]->len;
	assert(sendCursor >= 0);
}

bool RadioInterface::activateChan(int num)
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

bool RadioInterface::deactivateChan(int num)
{
	if ((num >= mChanM) || (num < 0)) {
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
