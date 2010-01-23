#  Copyright 2005 Tuukka Pasanen
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHE DEALINGS IN THE SOFTWARE.
#
# Process this file with autoconf to produce a configure script

AC_PREREQ(2.59)
AC_INIT([xf86-input-evtouch],[0.8.7],[http://www.conan.de/touchscreen/evtouch.html],[xf86-input-evtouch])

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE([dist-bzip2])

AM_MAINTAINER_MODE

DRIVER_NAME=evtouch
AC_SUBST([DRIVER_NAME])

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AM_PROG_CC_C_O

AH_TOP([#include "xorg-server.h"])

AC_ARG_WITH(xorg-module-dir,
            AS_HELP_STRING(--with-xorg-module-dir=DIR,Default xorg module directory [[[default=$libdir/xorg/modules]]]),
            [moduledir="$withval"],
            [moduledir="$libdir/xorg/modules"])
inputdir=${moduledir}/input
AC_SUBST(inputdir)

# Checks for pkg-config packages
PKG_CHECK_MODULES(XORG, xorg-server xproto $REQUIRED_MODULES)

# Install UDEV to etc dir
AC_ARG_ENABLE(udevinstall,
	     AS_HELP_STRING(--enable-udevinstall,Install udev to /etc/udev/rules.d/ [[[default=disabled]]]),
	     [	case "${enableval}" in
			yes|true) enable_udevinstall=true ;;
			no|false) enable_udevinstall=false ;;
			*) AC_MSG_ERROR([Bad value ${enableval} for --enable-udevinstall]) ;;
		esac],
	     [enable_udevinstall=false])

AM_CONDITIONAL(ENABLE_UDEVINSTALL, test "$enable_udevinstall" = "true")

# calibrator build
AC_ARG_ENABLE(calibrator,
	     AS_HELP_STRING(--enable-calibrator,Enable calibrator build and install [[[default=disabled]]]),
	     [	case "${enableval}" in
			yes|true) enable_calibrator=true ;;
			no|false) enable_calibrator=false ;;
			*) AC_MSG_ERROR([Bad value ${enableval} for --enable-calibrator]) ;;
		esac],
	     [enable_calibrator=false])
AM_CONDITIONAL(ENABLE_EVCAL, test "$enable_calibrator" = "true")

if test "${enable_calibrator}" = "true" ; then
	# Checks for pkg-config packages
	PKG_CHECK_MODULES(CALIBRATOR, x11)

	CALIBRATOR_CFLAGS="$CALIBRATOR_CFLAGS $CFLAGS $XORG_CFLAGS -DCURSORDIRECTORY=\"\\\"$CURSORDIR\\\"\""
	AC_SUBST(CURSORDIR)
	AC_SUBST(CALIBRATOR_CFLAGS)
	AC_SUBST(CALIBRATOR_LDFLAGS)
fi

dnl AC_MSG_RESULT([yes])
sdkdir=$(pkg-config --variable=sdkdir xorg-server)

EVTOUCH_CFLAGS="$CFLAGS $XORG_CFLAGS "
AC_SUBST(EVTOUCH_CFLAGS)

AC_SUBST([MKDIR_P])

# Checks for header files.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
