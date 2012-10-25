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

#ifndef RADIOPARAMS_H
#define RADIOPARAMS_H

#include "config.h"

/* 
 * Options
 *
 * ENABLE_ALL_CHANS - set to transmit dummy bursts on all active channels
 */
//#define ENABLE_ALL_CHANS

/* Device settings */
#define DEVICE_TX_AMPL		0.5
#define SAMPSPERSYM		1
#define GSM_RATE		(1625e3 / 6)

/* Channelizer settings */
#define CHAN_MAX		10
#define CHAN_RATE		400e3
#define CHAN_FILT_LEN		12

/* GSM resampler settings */
#define GSM_RESAMP_INRATE	(65 * SAMPSPERSYM)
#define GSM_RESAMP_OUTRATE	(96)
#define GSM_RESAMP_FILT_LEN	CHAN_FILT_LEN

/* Device resampler settings */
#define DEV_RESAMP_INRATE	(64)
#define DEV_RESAMP_OUTRATE	(65)
#define DEV_RESAMP_FILT_LEN	12

/* Rate change block size matching */
#define CHAN_MULT		(2 * 4)
#define CHAN_INCHUNK		(GSM_RESAMP_INRATE * CHAN_MULT)
#define CHAN_OUTCHUNK		(GSM_RESAMP_OUTRATE * CHAN_MULT)

#define RESAMP_MULT		(3 * 4)
#define RESAMP_INCHUNK		(DEV_RESAMP_INRATE * RESAMP_MULT)
#define RESAMP_OUTCHUNK		(DEV_RESAMP_OUTRATE * RESAMP_MULT)

#ifndef MULTICHAN
#  ifndef RESAMPLE
#    undef CHAN_RATE
#    define CHAN_RATE		GSM_RATE
#  endif
#endif

double getRadioOffset(int chanM,
		      double chanRate = CHAN_RATE,
		      int sps = SAMPSPERSYM,
		      int resampFiltLen = DEV_RESAMP_FILT_LEN,
		      int chanFiltLen = CHAN_FILT_LEN);

int getChanPaths(int num);

#endif /* RADIOPARAMS_H */
