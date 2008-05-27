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

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)
AM_LIBADD = $(PTHREAD_LIBS)
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
	pulse.c \
	read-sound-file.c read-sound-file.h \
	read-vorbis.c read-vorbis.h \
	read-wav.c read-wav.h \
	sound-theme-spec.c sound-theme-spec.h \
	llist.h \
	macro.h \
	malloc.h
libcanberra_la_LIBADD = \
	$(PULSE_LIBS) \
	$(VORBIS_LIBS)
libcanberra_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(PULSE_CFLAGS) \
	$(VORBIS_CFLAGS)

test_canberra_SOURCES = \
        test-canberra.c
test_canberra_LDADD = \
        $(AM_LDADD) \
        libcanberra.la
