# $Id$
#
# This file is part of libcanberra.
#
# Copyright 2008 Lennart Poettering
#
# libcanberra is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 2.1 of the
# License, or (at your option) any later version.
#
# libcanberra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with libcanberra. If not, If not, see
# <http://www.gnu.org/licenses/>.

moduledir = @GTK_MODULES_DIR@

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LDADD = $(PTHREAD_LIBS)

lib_LTLIBRARIES = \
        libcanberra.la

noinst_PROGRAMS = \
	test-canberra

libcanberra_la_SOURCES = \
	canberra.h \
	common.c common.h \
	mutex-posix.c mutex.h \
	proplist.c proplist.h \
	driver.h \
	read-sound-file.c read-sound-file.h \
	read-vorbis.c read-vorbis.h \
	read-wav.c read-wav.h \
	sound-theme-spec.c sound-theme-spec.h \
	llist.h \
	macro.h \
	malloc.c malloc.h
libcanberra_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VORBIS_CFLAGS)
libcanberra_la_LIBADD = \
	$(VORBIS_LIBS)

if HAVE_PULSE
if BUILTIN_PULSE

libcanberra_la_SOURCES += \
	pulse.c
libcanberra_la_CFLAGS += \
	$(PULSE_CFLAGS)
libcanberra_la_LIBADD += \
	$(PULSE_LIBS)

else

lib_LTLIBRARIES += \
	libcanberra-pulse.la

libcanberra_pulse_la_SOURCES = \
	pulse.c
libcanberra_pulse_la_CFLAGS = \
	$(PULSE_CFLAGS)
libcanberra_pulse_la_LIBADD = \
	$(PULSE_LIBS)

endif
endif

if HAVE_ALSA
if BUILTIN_ALSA

libcanberra_la_SOURCES += \
	alsa.c
libcanberra_la_CFLAGS += \
	$(ALSA_CFLAGS)
libcanberra_la_LIBADD += \
	 $(ALSA_LIBS)

else

lib_LTLIBRARIES += \
	libcanberra-alsa.la

libcanberra_alsa_la_SOURCES = \
	alsa.c
libcanberra_alsa_la_CFLAGS = \
	$(ALSA_CFLAGS)
libcanberra_alsa_la_LIBADD = \
	$(ALSA_LIBS)

endif
endif

if HAVE_NULL
if BUILTIN_NULL

libcanberra_la_SOURCES += \
	null.c

else

lib_LTLIBRARIES += \
	libcanberra-null.la

libcanberra_null_la_SOURCES = \
	null.c
endif
endif

if BUILTIN_DSO

libcanberra_la_SOURCES += \
	dso.c
libcanberra_la_CFLAGS += \
	$(LTDLINCL)
libcanberra_la_LIBADD += \
	$(LIBLTDL)

endif

if HAVE_GTK

lib_LTLIBRARIES += \
	libcanberra-gtk.la

module_LTLIBRARIES = \
	libcanberra-gtk-module.la

libcanberra_gtk_la_SOURCES = \
	canberra-gtk.h \
	canberra-gtk.c
libcanberra_gtk_la_CFLAGS = \
	$(GTK_CFLAGS)
libcanberra_gtk_la_LIBADD = \
	$(GTK_LIBS) \
	libcanberra.la

libcanberra_gtk_module_la_SOURCES = \
	canberra-gtk-module.c
libcanberra_gtk_module_la_CFLAGS = \
	$(GTK_CFLAGS)
libcanberra_gtk_module_la_LIBADD = \
	$(GTK_LIBS) \
	libcanberra.la \
	libcanberra-gtk.la
libcanberra_gtk_module_la_LDFLAGS = \
	-avoid-version -module -export-dynamic

endif

test_canberra_SOURCES = \
        test-canberra.c
test_canberra_LDADD = \
        $(AM_LDADD) \
        libcanberra.la
