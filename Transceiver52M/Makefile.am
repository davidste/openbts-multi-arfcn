#
# Copyright 2008 Free Software Foundation, Inc.
# Copyright 2010 Range Networks, Inc.
#
# This software is distributed under the terms of the GNU Public License.
# See the COPYING file in the main directory for details.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include $(top_srcdir)/Makefile.common

#UHD wins if both are defined
if UHD
AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES) $(UHD_CFLAGS)
else
if USRP1
AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES) $(USRP_CFLAGS)
else
#we should never be here, as this doesn't build if one of the above
#doesn't exist
AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES)
endif
endif
AM_CXXFLAGS = -ldl -lpthread

rev2dir = $(datadir)/usrp/rev2
rev4dir = $(datadir)/usrp/rev4

dist_rev2_DATA = std_inband.rbf
dist_rev4_DATA = std_inband.rbf

SUBDIRS = sigproc

SIGPROC_LA = $(top_builddir)/Transceiver52M/sigproc/libsigproc.la

EXTRA_DIST = \
	README \
	README.Talgorithm

noinst_LTLIBRARIES = libtransceiver.la

COMMON_SOURCES = \
	radioInterface.cpp \
	radioVector.cpp \
	radioClock.cpp \
	sigProcLib.cpp \
	Transceiver.cpp \
	DriveLoop.cpp \
	DummyLoad.cpp

if MULTICHAN 
libtransceiver_la_SOURCES = \
	$(COMMON_SOURCES) \
	radioIOMulti.cpp \
	ChannelizerBase.cpp \
	Channelizer.cpp \
	Synthesis.cpp \
	Resampler.cpp
else
if RESAMPLE
libtransceiver_la_SOURCES = \
	$(COMMON_SOURCES) \
	radioIOResamp.cpp \
	Resampler.cpp
else
libtransceiver_la_SOURCES = \
	$(COMMON_SOURCES) \
	radioIO.cpp
endif
endif

noinst_PROGRAMS = \
	transceiver \
	sigProcLibTest

noinst_HEADERS = \
	Complex.h \
	radioInterface.h \
	radioVector.h \
	radioClock.h \
	radioDevice.h \
	sigProcLib.h \
	Transceiver.h \
	USRPDevice.h \
	DummyLoad.h \
	rcvLPF_651.h \
	sendLPF_961.h \
	ChannelizerBase.h \
	Channelizer.h \
	Synthesis.h \
	Resampler.h

if MULTICHAN
transceiver_SOURCES = multiTRX.cpp
transceiver_LDADD = \
	libtransceiver.la \
	$(GSM_LA) \
	$(COMMON_LA) \
	$(SQLITE_LA) \
	$(SIGPROC_LA) \
	$(FFTW_LIBS)
else
transceiver_SOURCES = runTransceiver.cpp
transceiver_LDADD = \
	libtransceiver.la \
	$(GSM_LA) \
	$(COMMON_LA) \
	$(SQLITE_LA)
if RESAMPLE
transceiver_LDADD += $(SIGPROC_LA)
endif
endif

sigProcLibTest_SOURCES = sigProcLibTest.cpp
sigProcLibTest_LDADD = \
	libtransceiver.la \
	$(GSM_LA) \
	$(COMMON_LA) $(SQLITE_LA)

#uhd wins
if UHD
libtransceiver_la_SOURCES += UHDDevice.cpp
transceiver_LDADD += $(UHD_LIBS)
sigProcLibTest_LDADD += $(UHD_LIBS)
else
if USRP1
libtransceiver_la_SOURCES += USRPDevice.cpp
transceiver_LDADD += $(USRP_LIBS)
sigProcLibTest_LDADD += $(USRP_LIBS)
else
#we should never be here, as one of the above mustbe defined for us to build
endif
endif


MOSTLYCLEANFILES +=

#radioInterface.cpp
#ComplexTest.cpp
#sigProcLibTest.cpp
#sweepGenerator.cpp
#testRadio.cpp

