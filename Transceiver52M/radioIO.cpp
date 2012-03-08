/*
 * Radio device I/O interface
 * Written by Thomas Tsou <ttsou@vt.edu>
 *
 * Copyright 2011,2012 Free Software Foundation, Inc.
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

#include <radioInterface.h>
#include <Logger.h>

/* Receive a timestamped chunk from the device */ 
void RadioInterface::pullBuffer()
{
	bool local_underrun;

	/* Read samples. Fail if we don't get what we want. */
	int num_rd = mRadio->readSamples(&rcvBuffer[0][2 * rcvCursor],
					 OUTCHUNK, &overrun,
					 readTimestamp, &local_underrun);

	LOG(DEBUG) << "Rx read " << num_rd << " samples from device";
	assert(num_rd == OUTCHUNK);

	underrun |= local_underrun;
	readTimestamp += (TIMESTAMP) num_rd;

	rcvCursor += num_rd;
}

/* Send timestamped chunk to the device with arbitrary size */ 
void RadioInterface::pushBuffer()
{
	if (sendCursor < INCHUNK)
		return;

	/* Write samples. Fail if we don't get what we want. */
	int num_smpls = mRadio->writeSamples(sendBuffer[0],
					     sendCursor,
					     &underrun,
					     writeTimestamp);
	assert(num_smpls == sendCursor);

	writeTimestamp += (TIMESTAMP) num_smpls;
	sendCursor = 0;
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

bool RadioInterface::init()
{
}

void RadioInterface::close()
{
}
