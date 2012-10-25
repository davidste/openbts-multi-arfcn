/*
 * Radio configuration parameters
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

#include "radioParams.h"

#define NUM_CFGS		22

struct radioParam {
	int numChans;
	double chanRate;
	int sps;
	int resampFiltLen;
	int chanFiltLen;
};

struct offsetMap {
	radioParam param;
	double offset;
};

/*
 * Map various configuration options to a receive timing offset
 */
static offsetMap offsetTable[NUM_CFGS] = {
	/* 4 channels at 400 kHz spacing */
	{ { 4, 400e3, 1, 12, 12 }, 5.7373e-5 },
	{ { 4, 400e3, 1, 16, 16 }, 6.7214e-5 },

	/* 8 channels at 400 kHz spacing */
	{ { 8, 400e3, 1, 12, 12 }, 4.4136e-5 },
	{ { 8, 400e3, 1, 16, 16 }, 5.4869e-5 },
};

static bool cmpParam(radioParam a, radioParam b)
{
	if (a.numChans == 1)
		a.chanFiltLen = 0;

	if (a.chanRate == GSM_RATE)
		a.resampFiltLen = 0;

	if ((a.numChans == b.numChans) &&
	    (a.sps == b.sps) &&
	    (a.chanFiltLen == b.chanFiltLen) &&
	    (a.resampFiltLen == b.resampFiltLen)) {
		return true;
	}

	return false;
}

/*
 * Attempt to match the defined parameters with a known timing
 * offset value. If not found, return zero.
 */
double getRadioOffset(int numChans, double rate, int sps,
		      int resampFiltLen, int chanFileLen)
{
	int i;

	radioParam param = {
		numChans,
		rate,
		sps,
		resampFiltLen,
		chanFileLen
	};

	for (i = 0; i < NUM_CFGS; i++) {
		if (cmpParam(param, offsetTable[i].param))
			return offsetTable[i].offset;
	}

	return 0.0f;
}

int getChanPaths(int num)
{
	switch (num) {
	case 1:
	case 2:
	case 3:
		return 4;
	case 4:
	case 5:
	case 6:
	case 7:
		return 8;
	default:
		return -1;
	}
}
