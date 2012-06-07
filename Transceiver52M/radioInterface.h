/*
* Copyright 2008, 2012 Free Software Foundation, Inc.
*
* This software is distributed under multiple licenses; see the COPYING file in the main directory for licensing information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#ifndef _RADIOINTEFACE_H_
#define _RADIOINTEFACE_H_

#include "sigProcLib.h"  
#include "GSMCommon.h"
#include "LinkedLists.h"
#include "radioDevice.h"
#include "radioVector.h"
#include "radioClock.h"
#include "radioParams.h"

#define INCHUNK    (625)
#define OUTCHUNK   (625)

/** class to interface the transceiver with the USRP */
class RadioInterface {

private:

  int mChanM;                                 ///< channelizer width
  Thread *mAlignRadioServiceLoopThread;	      ///< thread that synchronizes transmit and receive sections

  VectorFIFO mReceiveFIFO[CHAN_MAX];	      ///< FIFO that holds receive  bursts

  RadioDevice *mRadio;			      ///< the USRP object
 
  float *sendBuffer[CHAN_MAX];
  unsigned sendCursor;

  float *rcvBuffer[CHAN_MAX];
  unsigned rcvCursor;

  bool chanActive[CHAN_MAX];
 
  bool underrun;			      ///< indicates writes to USRP are too slow
  bool overrun;				      ///< indicates reads from USRP are too slow
  TIMESTAMP writeTimestamp;		      ///< sample timestamp of next packet written to USRP
  TIMESTAMP readTimestamp;		      ///< sample timestamp of next packet read from USRP

  RadioClock mClock;                          ///< the basestation clock!

  int samplesPerSymbol;			      ///< samples per GSM symbol
  int receiveOffset;                          ///< offset b/w transmit and receive GSM timestamps, in timeslots
  int mRadioOversampling;
  int mTransceiverOversampling;

  bool mOn;				      ///< indicates radio is on

  double powerScaling;

  bool loadTest;
  int mNumARFCNs;
  signalVector *finalVec, *finalVec9;

  /** initialize I/O internals */
  bool init();

  /** format samples to USRP */ 
  int radioifyVector(signalVector &wVector,
                     float *floatVector,
                     float scale,
                     bool zero);

  /** format samples from USRP */
  int unRadioifyVector(float *floatVector, int offset, signalVector &wVector);

  /** push GSM bursts into the transmit buffer */
  void pushBuffer(void);

  /** pull GSM bursts from the receive buffer */
  void pullBuffer(void);

  /** load receive vectors into FIFO's */
  void loadVectors(unsigned tN, int samplesPerBurst, int index, GSM::Time rxClock);

public:

  /** start the interface */
  bool start();
  bool stop();

  bool started() { return mOn; };

  /** shutdown interface */
  void close();

  /** constructor */
  RadioInterface(RadioDevice* wRadio = NULL,
		 int wChanM = 1,
		 int receiveOffset = 3,
		 int wRadioOversampling = SAMPSPERSYM,
		 int wTransceiverOversampling = SAMPSPERSYM,
		 GSM::Time wStartTime = GSM::Time(0));
    
  /** destructor */
  ~RadioInterface();

  void setSamplesPerSymbol(int wSamplesPerSymbol) {if (!mOn) samplesPerSymbol = wSamplesPerSymbol;}

  int getSamplesPerSymbol() { return samplesPerSymbol;}

  /** check for underrun, resets underrun value */
  bool isUnderrun();
  
  /** return the receive FIFO */
  VectorFIFO* receiveFIFO(int num) { return &mReceiveFIFO[num];}

  /** return the basestation clock */
  RadioClock* getClock(void) { return &mClock;};

  /** set transmit frequency */
  bool tuneTx(double freq);

  /** set receive frequency */
  bool tuneRx(double freq);

  /** set receive gain */
  double setRxGain(double dB);

  /** get receive gain */
  double getRxGain(void);

  /** drive transmission of GSM bursts */
  void driveTransmitRadio(signalVector **radioBurst, bool *zeroBurst);

  /** drive reception of GSM bursts */
  void driveReceiveRadio();

  void setPowerAttenuation(double atten);

  /** returns the full-scale transmit amplitude **/
  double fullScaleInputValue();

  /** returns the full-scale receive amplitude **/
  double fullScaleOutputValue();

  /** set thread priority on current thread */
  void setPriority() { mRadio->setPriority(); }

  /** get transport bus type of attached device */ 
  enum RadioDevice::busType getBus() { return mRadio->getBus(); }

  /** activate a channel */
  bool activateChan(int num);

  /** deactivate a channel */
  bool deactivateChan(int num);

protected:

  /** drive synchronization of Tx/Rx of USRP */
  void alignRadio();

  /** reset the interface */
  void reset();

  /** interface status */
  bool on() { return mOn; }

  friend void *AlignRadioServiceLoopAdapter(RadioInterface*);

};

/** synchronization thread loop */
void *AlignRadioServiceLoopAdapter(RadioInterface*);

#endif /* _RADIOINTEFACE_H_ */
