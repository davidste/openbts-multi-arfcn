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
	/* Single 270.83333 kHz channel */
	{ { 1, GSM_RATE, 1, 0, 0 }, 0.0f },
	{ { 1, GSM_RATE, 2, 0, 0 }, 0.0f },

	/* Single 400 kHz channel */
	{ { 1, 400e3, 1, 8,  0 }, 45.3365e-6 },
	{ { 1, 400e3, 1, 16, 0 }, 40.9651e-6 },
	{ { 1, 400e3, 2, 8,  0 }, 53.1899e-6 },
	{ { 1, 400e3, 2, 16, 0 }, 55.7547e-6 },

	/* 5 channels at 400 kHz spacing */
	{ { 5, 400e3, 1, 8,  8  }, 28.0889e-6 },
	{ { 5, 400e3, 1, 8,  16 }, 48.0096e-6 },
	{ { 5, 400e3, 1, 16, 8  }, 23.1226e-6 },
	{ { 5, 400e3, 1, 16, 16 }, 42.9651e-6 },
	{ { 5, 400e3, 2, 8,  8  }, 35.4855e-6 },
	{ { 5, 400e3, 2, 8,  16 }, 55.6850e-6 },
	{ { 5, 400e3, 2, 16, 8  }, 38.4591e-6 },
	{ { 5, 400e3, 2, 16, 16 }, 58.2836e-6 },

	/* 10 channels at 400 kHz spacing */
	{ { 10, 400e3, 1, 8,  8  }, 22.4399e-6 },
	{ { 10, 400e3, 1, 8,  16 }, 42.3846e-6 },
	{ { 10, 400e3, 1, 16, 8  }, 17.6044e-6 },
	{ { 10, 400e3, 1, 16, 16 }, 37.6947e-6 },
	{ { 10, 400e3, 2, 8,  8  }, 29.8060e-6 },
	{ { 10, 400e3, 2, 8,  16 }, 50.0241e-6 },
	{ { 10, 400e3, 2, 16, 8  }, 32.6202e-6 },
	{ { 10, 400e3, 2, 16, 16 }, 52.7403e-6 },
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
		return 3;
	case 2:
	case 3:
		return 4;
	case 4:
	case 5:
		return 6;
	case 6:
	case 7:
		return 10;
	case 8:
	case 9:
		return 12;
	case 10:
	case 11:
		return 15;
	default:
		return -1;
	}
}
