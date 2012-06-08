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

/* Device settings */
#define DEVICE_TX_AMPL		0.5
#define SAMPSPERSYM		1
#define GSM_RATE		(1625e3 / 6)

/* Channelizer settings */
#define CHAN_MAX		10
#define CHAN_RATE		400e3
#define CHAN_FILT_LEN		16

/* Resampler settings */
#define RESAMP_INRATE		(65 * SAMPSPERSYM * SAMPSPERSYM)
#define RESAMP_MIDRATE		(96 * SAMPSPERSYM)
#define RESAMP_OUTRATE		(104 * SAMPSPERSYM)
#define RESAMP_FILT_LEN		8

#ifndef MULTICHAN
#  ifndef RESAMPLE
#    undef CHAN_RATE
#    define CHAN_RATE		GSM_RATE
#  endif
#endif

double getRadioOffset(int chanM,
		      double chanRate = CHAN_RATE,
		      int sps = SAMPSPERSYM,
		      int resampFiltLen = RESAMP_FILT_LEN,
		      int chanFiltLen = CHAN_FILT_LEN);

#endif /* RADIOPARAMS_H */
