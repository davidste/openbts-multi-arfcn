/*
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

#include <time.h>
#include <signal.h>

#include <GSMCommon.h>
#include <Logger.h>
#include <Configuration.h>

#include "Transceiver.h"
#include "radioDevice.h"

ConfigurationTable gConfig("/etc/OpenBTS/OpenBTS.db");

volatile bool gbShutdown = false;

int Transceiver::mTSC = 0;

static void sigHandler(int signum)
{
	LOG(NOTICE) << "Received shutdown signal";
	gbShutdown = true;
}

static int setupSignals()
{
	struct sigaction action;

	action.sa_handler = sigHandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	if (sigaction(SIGINT, &action, NULL) < 0)
		return -1;

	if (sigaction(SIGTERM, &action, NULL) < 0)
		return -1;

	return 0;
}

/*
 * Generate the channel-transceiver ordering. Attempt to match the RAD1
 * ordering where the active channels are centered in the overall device
 * bandwidth. C0 is always has the lowest ARFCN with increasing subsequent
 * channels. When an even number of channels is selected, the carriers will
 * be offset from the RF center by -200 kHz, or half ARFCN spacing.
 */
static void genChanMap(int numARFCN, int chanM, int *chans)
{
	int i, n;

	chans[0] = numARFCN / 2; 

	for (i = 1, n = 1; i < numARFCN; i++) {
		if (!chans[i - 1])
			chans[i] = chanM - 1;
		else
			chans[i] = chans[i - 1] - 1;
	}
}

static void createTrx(Transceiver **trx, int *map, int num,
		      RadioInterface *radio, DriveLoop *drive)
{
	int i;
	bool primary = true;

	for (i = 0; i < num; i++) {
		LOG(NOTICE) << "Creating TRX" << i
			    << " attached on channel " << map[i];

		radio->activateChan(map[i]);
		trx[i] = new Transceiver(5700 + 2 * i, "127.0.0.1",
					 SAMPSPERSYM, radio, drive,
					 map[i], primary);
		trx[i]->start();
		primary = false;
	}
}

int main(int argc, char *argv[])
{
	int i, chanM, numARFCN = 1;
	int chanMap[CHAN_MAX];
	RadioDevice *usrp;
	RadioInterface* radio;
	DriveLoop *drive;
	Transceiver *trx[CHAN_MAX];

	gLogInit("transceiver", gConfig.getStr("Log.Level").c_str(), LOG_LOCAL7);

	if (argc > 1) {
		numARFCN = atoi(argv[1]);
		if (numARFCN > (CHAN_MAX - 1)) {
			LOG(ALERT) << numARFCN << " channels not supported with current build";
			exit(-1);
		}
	}

	srandom(time(NULL));

	if (setupSignals() < 0) {
		LOG(ERR) << "Failed to setup signal handlers, exiting...";
		exit(-1);
	}

	/*
	 * Select the number of channels according to the number of ARFCNs's
	 * and generate ARFCN-to-channelizer path mappings. The channelizer
	 * aliases and extracts 'M' equally spaced channels to baseband. The
	 * number of ARFCN's must be less than the number of channels in the
	 * channelizer.
	 */
	switch (numARFCN) {
	default:
		chanM = 8;
	}
	genChanMap(numARFCN, chanM, chanMap);

	/* Find a timing offset based on the channelizer configuration */
	double rxOffset = getRadioOffset(chanM);
	if (rxOffset == 0.0f) {
		LOG(ALERT) << "Rx sample offset not found, using offset of 0.0s";
		LOG(ALERT) << "Rx burst timing may not be accurate"; 
	}

	double deviceRate = chanM * CHAN_RATE * OUTER_RESAMP_OUTRATE / OUTER_RESAMP_INRATE;
	usrp = RadioDevice::make(deviceRate, rxOffset, DEVICE_TX_AMPL / numARFCN);
	if (!usrp->open()) {
		LOG(ALERT) << "Failed to open device, exiting...";
		return EXIT_FAILURE;
	}

	radio = new RadioInterface(usrp, chanM, 3, SAMPSPERSYM, 0, false);
	drive = new DriveLoop(5700, "127.0.0.1", chanM, chanMap[0],
			      SAMPSPERSYM, GSM::Time(6,0), radio);

	/* Create, attach, and activate all transceivers */
	createTrx(trx, chanMap, numARFCN, radio, drive);

	while (!gbShutdown) { 
		sleep(1);
	}

	LOG(NOTICE) << "Shutting down transceivers...";
	for (i = 0; i < numARFCN; i++) {
		trx[i]->shutdown();
	}

	/*
	 * Allow time for threads to end before we start freeing objects
	 */
	sleep(2);

	for (i = 0; i < numARFCN; i++) {
		delete trx[i];
	}

	delete drive;
	delete radio;
	delete usrp;
}
