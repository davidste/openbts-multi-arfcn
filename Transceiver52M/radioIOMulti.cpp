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
#define INCHUNK			(RESAMP_INRATE * CHUNKMUL)
#define MIDCHUNK		(RESAMP_MIDRATE * CHUNKMUL)
#define OUTCHUNK		(RESAMP_OUTRATE * CHUNKMUL)
#define INBUFLEN		(INCHUNK * 4)
#define MIDBUFLEN		(MIDCHUNK * 4)
#define OUTBUFLEN		(OUTCHUNK * 4)

static struct cxvec *highRateTxBuf;
static struct cxvec *highRateRxBuf;

static struct cxvec *midRateTxBuf;
static struct cxvec *midRateRxBuf;

static struct cxvec *lowRateTxBufs[CHAN_MAX];
static struct cxvec *lowRateRxBufs[CHAN_MAX];

static Resampler *upsampler;
static Resampler *dnsampler;
static Channelizer *chan;
static Synthesis *synth;

/* Initialize I/O specific objects */
bool RadioInterface::init()
{
	int i;

        dnsampler = new Resampler(RESAMP_MIDRATE, RESAMP_OUTRATE, RESAMP_FILT_LEN, 1);
        if (!dnsampler->init(NULL)) {
                LOG(ALERT) << "Rx resampler failed to initialize";
                return false;
        }
        dnsampler->activateChan(0);

        upsampler = new Resampler(RESAMP_OUTRATE, RESAMP_MIDRATE, RESAMP_FILT_LEN, 1);
        if (!upsampler->init(NULL)) {
                LOG(ALERT) << "Tx resampler failed to initialize";
                return false;
        }
        upsampler->activateChan(0);

	chan = new Channelizer(mChanM, CHAN_FILT_LEN, RESAMP_FILT_LEN,
			       RESAMP_INRATE, RESAMP_MIDRATE, CHUNKMUL);
	if (!chan->init(NULL)) {
		LOG(ALERT) << "Rx channelizer failed to initialize";
		return false;
	}

	synth = new Synthesis(mChanM, CHAN_FILT_LEN, RESAMP_FILT_LEN,
			      RESAMP_MIDRATE, RESAMP_INRATE, CHUNKMUL);
	if (!synth->init(NULL)) {
		LOG(ALERT) << "Tx channelizer failed to initialize";
		return false;
	}

	highRateTxBuf = cxvec_alloc(OUTBUFLEN * mChanM, 0, NULL, 0);
	highRateRxBuf = cxvec_alloc(OUTBUFLEN * mChanM + RESAMP_FILT_LEN, RESAMP_FILT_LEN, NULL, 0);
	midRateTxBuf = cxvec_alloc(MIDBUFLEN * mChanM + RESAMP_FILT_LEN, RESAMP_FILT_LEN, NULL, 0);
	midRateRxBuf = cxvec_alloc(MIDBUFLEN * mChanM, 0, NULL, 0);

	midRateTxBuf->len = MIDCHUNK * mChanM;
	midRateRxBuf->len = MIDCHUNK * mChanM;

	/*
	 * Setup per-channel variables. The low rate transmit vectors 
	 * feed into the resampler prior to the synthesis filter
	 * and requires headroom equivalent to the filter length. Low
	 * rate buffers are allocated in the main radio interface code.
	 */
	for (i = 0; i < mChanM; i++) {
		if (chanActive[i]) {
			chan->activateChan(i);
			synth->activateChan(i);
		}

		lowRateRxBufs[i] =
			cxvec_alloc(2 * 625, 0, (cmplx *) rcvBuffer[i], 0);
		lowRateTxBufs[i] =
			cxvec_alloc(2 * 625, RESAMP_FILT_LEN, (cmplx *) sendBuffer[i], 0);
	}

	return true;
}

void RadioInterface::close()
{
	int i;

	cxvec_free(highRateTxBuf);
	cxvec_free(highRateRxBuf);

	/* Don't deallocate class member buffers */
	for (i = 0; i < mChanM; i ++) {
		lowRateRxBufs[i]->buf = NULL;
		lowRateTxBufs[i]->buf = NULL;

		cxvec_free(lowRateRxBufs[i]);
		cxvec_free(lowRateTxBufs[i]);
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
	numRead = mRadio->readSamples((float *) highRateRxBuf->data,
				      OUTCHUNK * mChanM, &overrun,
				      readTimestamp, &localUnderrun);

	assert(numRead == (OUTCHUNK * mChanM));

	highRateRxBuf->len = numRead;
	underrun |= localUnderrun;
	readTimestamp += (TIMESTAMP) highRateRxBuf->len;

	for (i = 0; i < mChanM; i++) {
		lowRateRxBufs[i]->start_idx = rcvCursor;
		lowRateRxBufs[i]->data = &lowRateRxBufs[i]->buf[rcvCursor];
		lowRateRxBufs[i]->len = INCHUNK;
	}

	/* Channelize */
	midRateTxBuf->len = MIDCHUNK * mChanM;
	numConverted = dnsampler->rotate(&highRateRxBuf, &midRateRxBuf);
	numConverted = chan->rotate(midRateRxBuf, lowRateRxBufs);
	rcvCursor += numConverted;
}

/* Resize data buffers of length 'len' after reading 'n' samples */
static void shiftTxBufs(struct cxvec **vecs, int chanM, int len, int n)
{
	int i;

	for (i = 0; i < chanM; i++) {
		memmove(vecs[i]->data, &vecs[i]->data[n],
			(len - n) * sizeof(cmplx));
	}
}

/* Send a timestamped chunk to the device */
void RadioInterface::pushBuffer()
{
	int i, numConverted, numChunks, numSent;

	if (sendCursor < INCHUNK)
		return;

	/* We only handle 1 */
	numChunks = 1;

	for (i = 0; i < mChanM; i++) {
		lowRateTxBufs[i]->len = numChunks * INCHUNK;
	}

	midRateTxBuf->len = numChunks * MIDCHUNK * mChanM;
	highRateTxBuf->len = numChunks * OUTCHUNK * mChanM;

	numConverted = synth->rotate(lowRateTxBufs, midRateTxBuf);
	numConverted = upsampler->rotate(&midRateTxBuf, &highRateTxBuf);

	/* Write samples. Fail if we don't get what we want. */
	numSent = mRadio->writeSamples((float *) highRateTxBuf->data,
					numConverted,
					&underrun,
					writeTimestamp);
	assert(numSent == numConverted);
	writeTimestamp += (TIMESTAMP) numSent;

	/* Move unsent samples to beginning of buffer */
	shiftTxBufs(lowRateTxBufs, mChanM, sendCursor, lowRateTxBufs[0]->len);
	sendCursor -= lowRateTxBufs[0]->len;
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
