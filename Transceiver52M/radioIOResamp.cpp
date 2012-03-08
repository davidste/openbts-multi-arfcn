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
#define INRATE       65 * SAMPSPERSYM
#define INHISTORY    INRATE * 2
#define INCHUNK      INRATE * 9

#define OUTRATE      96 * SAMPSPERSYM
#define OUTHISTORY   OUTRATE * 2
#define OUTCHUNK     OUTRATE * 9

#define FILT_LEN     10

static struct cxvec *hr_tx_vec;
static struct cxvec *hr_rx_vec;

static struct cxvec *lr_tx_vec;
static struct cxvec *lr_rx_vec;

static Resampler *upsampler;
static Resampler *dnsampler;

/* Initialize I/O specific objects */
bool RadioInterface::init()
{
	assert(CHAN_M == 1);
	assert(chanActive[0]);

	dnsampler = new Resampler(INRATE, OUTRATE, FILT_LEN, 1);
	if (!dnsampler->init(NULL)) {
		LOG(ALERT) << "Rx resampler failed to initialize";
		return false; 
	}
	dnsampler->activateChan(0);

	upsampler = new Resampler(OUTRATE, INRATE, FILT_LEN, 1);
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
	hr_tx_vec = cxvec_alloc(INCHUNK * 4, 0, NULL, 0);
	hr_rx_vec = cxvec_alloc(OUTCHUNK * 4, FILT_LEN, NULL, 0);
	lr_rx_vec = cxvec_alloc(8 * 625, 0, (cmplx *) rcvBuffer[0], 0);
	lr_tx_vec = cxvec_alloc(8 * 625, FILT_LEN, (cmplx *) sendBuffer[0], 0);

	return true;
}

/* Shutdown I/O specific objects */
void RadioInterface::close()
{
	cxvec_free(hr_tx_vec);
	cxvec_free(hr_rx_vec);

	/* Deallocate class member buffers in the destructor */
	lr_rx_vec->buf = NULL;
	lr_tx_vec->buf = NULL;

	cxvec_free(lr_rx_vec);
	cxvec_free(lr_tx_vec);

	delete upsampler;
	delete dnsampler;
}

/* Receive a timestamped chunk from the device */
void RadioInterface::pullBuffer()
{
	int num_cv, num_rd;
	bool local_underrun;

	/* Read samples. Fail if we don't get what we want. */
	hr_rx_vec->len = mRadio->readSamples((float *) hr_rx_vec->data,
					     OUTCHUNK, &overrun, readTimestamp,
					     &local_underrun);

	LOG(DEBUG) << "Rx read " << hr_rx_vec->len << " samples from device";
	assert(hr_rx_vec->len == OUTCHUNK);

	underrun |= local_underrun;
	readTimestamp += (TIMESTAMP) hr_rx_vec->len;

	lr_rx_vec->start_idx = rcvCursor;
	lr_rx_vec->data = &lr_rx_vec->buf[rcvCursor];
	lr_rx_vec->len = INCHUNK;

	num_cv = dnsampler->rotate(&hr_rx_vec, &lr_rx_vec);

	rcvCursor += num_cv;
}

/* Resize data buffers of length 'len' after reading 'n' samples */
static void shift_tx_bufs(struct cxvec *vec, int len, int n)
{
	memmove(vec->data, &vec->data[n], (len - n) * sizeof(cmplx));
}

/* Send a timestamped chunk to the device */
void RadioInterface::pushBuffer()
{
	int i, num_cv, num_chunks;

	if (sendCursor < INCHUNK)
		return;

	/* Only handle 1 for now */
	num_chunks = 1;
	lr_tx_vec->len = num_chunks * INCHUNK;
	hr_tx_vec->len = num_chunks * OUTCHUNK;

	num_cv = upsampler->rotate(&lr_tx_vec, &hr_tx_vec);

	/* Write samples. Fail if we don't get what we want. */
	int num_smpls = mRadio->writeSamples((float *) hr_tx_vec->data,
					     num_cv,
					     &underrun,
					     writeTimestamp);
	assert(num_smpls == num_cv);

	writeTimestamp += (TIMESTAMP) num_smpls;

	/* Move unsent samples to beginning of buffer */
	shift_tx_bufs(lr_tx_vec, sendCursor, lr_tx_vec->len);

	sendCursor -= lr_tx_vec->len;
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
