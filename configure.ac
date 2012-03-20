dnl
dnl Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
dnl
dnl This software is distributed under the terms of the GNU Public License.
dnl See the COPYING file in the main directory for details.
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl

AC_INIT(openbts,P2.8TRUNK)
AC_PREREQ(2.57)
AC_CONFIG_SRCDIR([config/Makefile.am])
AC_CONFIG_AUX_DIR([.])	
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

dnl Linux kernel KBuild style compile messages
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_PROG_AS
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PATH_PROG([RM_PROG], [rm])

AC_LIBTOOL_WIN32_DLL
AC_ENABLE_SHARED	dnl do build shared libraries
AC_DISABLE_STATIC	dnl don't build static libraries
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
dnl This is required for GnuRadio includes to understand endianess correctly:
AC_CHECK_HEADERS([byteswap.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_C_BIGENDIAN

AC_ARG_WITH(uhd, [
    AS_HELP_STRING([--with-uhd],
        [enable UHD based transceiver])
])

AC_ARG_WITH(resamp, [
    AS_HELP_STRING([--with-resamp],
        [enable resampling for non-52MHz devices])
])

AC_ARG_WITH(extref, [
    AS_HELP_STRING([--with-extref],
        [enable external reference on UHD devices])
])

AS_IF([test "x$with_uhd" = "xyes"],[
    PKG_CHECK_MODULES(UHD, uhd >= 003.002.000)
    AC_DEFINE(USE_UHD, 1, Define to 1 if using UHD)
])

AS_IF([test "x$with_resamp" = "xyes"], [
    AC_DEFINE(RESAMPLE, 1, Define to 1 for resampling)
])

AS_IF([test "x$with_extref" = "xyes"], [
    AC_DEFINE(EXTREF, 1, Define to 1 for external reference)
])

AM_CONDITIONAL(RESAMPLE, [test "x$with_resamp" = "xyes"])
AM_CONDITIONAL(UHD, [test "x$with_uhd" = "xyes"])
AM_CONDITIONAL(USRP1, [false])

# Defines OSIP_CFLAGS, OSIP_INCLUDEDIR, and OSIP_LIBS
PKG_CHECK_MODULES(OSIP, libosip2)

# Defines ORTP_CFLAGS, ORTP_INCLUDEDIR, and ORTP_LIBS
PKG_CHECK_MODULES(ORTP, ortp)

# Defines LIBUSB_TRANSFER_CANCELLED, LIBUSB_TRANSFER_COMPLETED, LIBUSB_SUCCESS, LIBUSB_ERROR_*
PKG_CHECK_MODULES(LIBUSB, libusb-1.0)

# Enable Multi-Channel
PKG_CHECK_MODULES(FFTW, fftw3f)
AC_DEFINE(MULTICHAN, 1, Define to 1 for multi-arfcn support)
AM_CONDITIONAL(MULTICHAN, [true])

# Check for SSE3 
NO_SSE3_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -msse3"

AC_MSG_CHECKING(for SSE3)
AC_LINK_IFELSE([
    AC_LANG_PROGRAM([[
#include <pmmintrin.h>
__m128 testfunc(float *a, float *b) {
    return _mm_hadd_ps(_mm_loadu_ps(a), _mm_loadu_ps(b));
}
    ]])
], [has_sse3=yes], [has_sse3=no])

AC_MSG_RESULT($has_sse3)

if test "$has_sse3" = yes; then
    AC_DEFINE(USE_SSE3, 1, [Enable SSE3 support])
else
    CFLAGS="$NO_SSE_CFLAGS"
fi

AM_CONDITIONAL(USE_SSE3, [test "x$has_sse3" = "xyes"])

# Prepends -lreadline to LIBS and defines HAVE_LIBREADLINE in config.h
AC_CHECK_LIB(readline, readline)

dnl Output files
AC_CONFIG_FILES([\
    Makefile \
    apps/Makefile \
    config/Makefile \
    doc/Makefile \
    tools/Makefile \
    AsteriskConfig/Makefile \
    CommonLibs/Makefile \
    Globals/Makefile \
    Control/Makefile \
    GSM/Makefile \
    SIP/Makefile \
    SMS/Makefile \
    TransceiverRAD1/Makefile \
    TRXManager/Makefile \
    CLI/Makefile \
    SR/Makefile \
    sqlite3/Makefile \
]) 

AS_IF([test "x$with_usrp1" = "xyes" -o "x$with_uhd" = "xyes"],
    [AC_CONFIG_FILES([\
        Transceiver52M/Makefile \
        Transceiver52M/sigproc/Makefile \
    ])
])

AC_OUTPUT
