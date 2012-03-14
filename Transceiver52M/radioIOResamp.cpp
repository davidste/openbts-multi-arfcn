/*
 * Radio device interface with sample rate conversion
 * Written by Thomas Tsou <ttsou@vt.edu>
 *
 * Copyright 2011 Free Software Foundation, Inc.
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

#include <Resampler.h>
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

/* Resampling parameters */
#define INHISTORY    RESAMP_INRATE * 2
#define INCHUNK      RESAMP_INRATE * 9
#define OUTHISTORY   RESAMP_OUTRATE * 2
#define OUTCHUNK     RESAMP_OUTRATE * 9

static struct cxvec *highRateTxBuf;
static struct cxvec *highRateRxBuf;

static struct cxvec *lowRateTxBuf;
static struct cxvec *lowRateRxBuf;

static Resampler *upsampler;
static Resampler *dnsampler;

/* Initialize I/O specific objects */
bool RadioInterface::init()
{
	assert(CHAN_M == 1);
	assert(chanActive[0]);

	dnsampler = new Resampler(RESAMP_INRATE, RESAMP_OUTRATE, RESAMP_FILT_LEN, 1);
	if (!dnsampler->init(NULL)) {
		LOG(ALERT) << "Rx resampler failed to initialize";
		return false; 
	}
	dnsampler->activateChan(0);

	upsampler = new Resampler(RESAMP_OUTRATE, RESAMP_INRATE, RESAMP_FILT_LEN, 1);
	if (!upsampler->init(NULL)) {
		LOG(ALERT) << "Tx resampler failed to initialize";
		return false;
	}
	upsampler->activateChan(0);

	/*
	 * Allocate high and low rate buffers. The high rate receive
	 * buffer and low rate transmit vectors feed into the resampler
	 * and requires headroom equivalent to the filter length. Low
	 * rate buffers are allocated in the main radio interface code.
	 */
	highRateTxBuf = cxvec_alloc(INCHUNK * 4, 0, NULL, 0);
	highRateRxBuf = cxvec_alloc(OUTCHUNK * 4, RESAMP_FILT_LEN, NULL, 0);

	lowRateRxBuf = cxvec_alloc(8 * 625, 0, (cmplx *) rcvBuffer[0], 0);
	lowRateTxBuf = cxvec_alloc(8 * 625, RESAMP_FILT_LEN, (cmplx *) sendBuffer[0], 0);

	return true;
}

/* Shutdown I/O specific objects */
void RadioInterface::close()
{
	cxvec_free(highRateTxBuf);
	cxvec_free(highRateRxBuf);

	/* Deallocate class member buffers in the destructor */
	lowRateRxBuf->buf = NULL;
	lowRateTxBuf->buf = NULL;

	cxvec_free(lowRateRxBuf);
	cxvec_free(lowRateTxBuf);

	delete upsampler;
	delete dnsampler;
}

/* Receive a timestamped chunk from the device */
void RadioInterface::pullBuffer()
{
	int numConverted, numRead;
	bool local_underrun;

	/* Read samples. Fail if we don't get what we want. */
	numRead = mRadio->readSamples((float *) highRateRxBuf->data,
				      OUTCHUNK, &overrun, readTimestamp,
				      &local_underrun);

	LOG(DEBUG) << "Rx read " << highRateRxBuf->len << " samples from device";
	assert(numRead == OUTCHUNK);

	highRateRxBuf->len = numRead;
	underrun |= local_underrun;
	readTimestamp += (TIMESTAMP) highRateRxBuf->len;

	lowRateRxBuf->start_idx = rcvCursor;
	lowRateRxBuf->data = &lowRateRxBuf->buf[rcvCursor];
	lowRateRxBuf->len = INCHUNK;

	numConverted = dnsampler->rotate(&highRateRxBuf, &lowRateRxBuf);
	rcvCursor += numConverted;
}

/* Resize data buffers of length 'len' after reading 'n' samples */
static void shiftTxBufs(struct cxvec *vec, int len, int n)
{
	memmove(vec->data, &vec->data[n], (len - n) * sizeof(cmplx));
}

/* Send a timestamped chunk to the device */
void RadioInterface::pushBuffer()
{
	int i, numConverted, numChunks, numSent;

	if (sendCursor < INCHUNK)
		return;

	/* Only handle 1 for now */
	numChunks = 1;
	lowRateTxBuf->len = numChunks * INCHUNK;
	highRateTxBuf->len = numChunks * OUTCHUNK;

	numConverted = upsampler->rotate(&lowRateTxBuf, &highRateTxBuf);

	/* Write samples. Fail if we don't get what we want. */
	numSent = mRadio->writeSamples((float *) highRateTxBuf->data,
					numConverted,
					&underrun,
					writeTimestamp);
	assert(numSent == numConverted);
	writeTimestamp += (TIMESTAMP) numSent;

	/* Move unsent samples to beginning of buffer */
	shiftTxBufs(lowRateTxBuf, sendCursor, lowRateTxBuf->len);
	sendCursor -= lowRateTxBuf->len;
	assert(sendCursor >= 0);
}

bool RadioInterface::activateChan(int num)
{
	if (num != 0) {
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
	if (num != 0) {
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
