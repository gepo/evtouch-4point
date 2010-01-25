#  Copyright 2005-2007 Tuukka Pasanen
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
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AM_CPPFLAGS = $(XORG_CFLAGS)

EXTRA_DIST = calibrator.c evtouch.h xf86OSmouse.h make_distrib.sh \
README.calibration 69-touchscreen.rules
noinst_HEADERS = evtouch.h 

if ENABLE_UDEVINSTALL
udevdir = @sysconfdir@/udev/rules.d
udev_DATA = 69-touchscreen.rules
endif

if ENABLE_EVCAL
pkglib_PROGRAMS = calibrator

calibrator_SOURCES = calibrator.c
calibrator_LDFLAGS = @CALIBRATOR_LIBS@ @CALIBRATOR_LDFLAGS@
calibrator_CFLAGS = @CALIBRATOR_CFLAGS@ 
endif

@DRIVER_NAME@_drv_la_LTLIBRARIES = @DRIVER_NAME@_drv.la
@DRIVER_NAME@_drv_la_LDFLAGS = -module -avoid-version
@DRIVER_NAME@_drv_la_CFLAGS = -DXINPUT @EVTOUCH_CFLAGS@
@DRIVER_NAME@_drv_ladir = @inputdir@

@DRIVER_NAME@_drv_la_SOURCES = @DRIVER_NAME@.c xf86OSmouse.h 
