# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

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
# License along with libcanberra. If not, see
# <http://www.gnu.org/licenses/>.

m4_define([ca_major],[0])
m4_define([ca_minor],[22])
m4_define([ca_version],[ca_major.ca_minor])

AC_PREREQ(2.63)

AC_INIT([libcanberra],[ca_version],[mzyvopnaoreen (at) 0pointer (dot) de])
AC_CONFIG_SRCDIR([src/common.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE([foreign 1.10 -Wall])

AC_SUBST(PACKAGE_URL, [http://0pointer.de/lennart/projects/libcanberra/])

AC_SUBST(LIBCANBERRA_VERSION_INFO, [2:1:2])
AC_SUBST(LIBCANBERRA_GTK_VERSION_INFO, [1:5:1])

AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED([CANONICAL_HOST], "$host", [Canonical host string.])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

if type -p stow > /dev/null && test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION} ***])
   ac_default_prefix="/usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION}"
fi

#### Checks for programs. ####

AC_PROG_MKDIR_P

# CC

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
AC_USE_SYSTEM_EXTENSIONS

# C++

AC_PROG_CXX

DESIRED_FLAGS="-Wall -W -Wextra -pedantic -pipe -Wno-long-long -Winline -Wvla -Wno-overlength-strings -Wunsafe-loop-optimizations -Wundef -Wformat=2 -Wlogical-op -Wsign-compare -Wpacked -Wformat-security -Wmissing-include-dirs -Wformat-nonliteral -Wold-style-definition -Wpointer-arith -Winit-self -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wmissing-noreturn -Wshadow -Wendif-labels -Wstrict-aliasing=2 -Wcast-align -Wwrite-strings -Wno-unused-parameter -ffast-math -Wp,-D_FORTIFY_SOURCE=2 -fno-common -fdiagnostics-show-option"

for flag in $DESIRED_FLAGS ; do
  CC_CHECK_CFLAGS([$flag], [CFLAGS="$CFLAGS $flag"])
done

#### libtool stuff ####
LT_PREREQ(2.2)
LT_INIT([dlopen win32-dll disable-static])

dnl Unfortunately, even up to libtool 2.2.6a there is no way to know
dnl exactly which version of libltdl is present in the system, so we
dnl just assume that it's a working version as long as we have the
dnl library and the header files.
dnl
dnl As an extra safety device, check for lt_dladvise_init() which is
dnl only implemented in libtool 2.x, and refine as we go if we have
dnl refined requirements.
dnl
dnl Check the header files first since the system may have a
dnl libltdl.so for runtime, but no headers, and we want to bail out as
dnl soon as possible.
dnl
dnl We don't need any special variable for this though, since the user
dnl can give the proper place to find libltdl through the standard
dnl variables like LDFLAGS and CPPFLAGS.

AC_CHECK_HEADER([ltdl.h],
    [AC_CHECK_LIB([ltdl], [lt_dladvise_init], [LIBLTDL=-lltdl], [LIBLTDL=])],
    [LIBLTDL=])

AS_IF([test "x$LIBLTDL" = "x"],
    [AC_MSG_ERROR([Unable to find libltdl.])])
AC_SUBST([LIBLTDL])

#### Determine build environment ####

os_is_win32=0

case "$host_os" in
     mingw*)
        AC_DEFINE([OS_IS_WIN32], 1, [Build target is Windows.])
        os_is_win32=1
     ;;
esac

AM_CONDITIONAL(OS_IS_WIN32, test "x$os_is_win32" = "x1")

AM_CONDITIONAL(USE_VERSION_SCRIPT, test "x$supports_anon_versioning" = "xyes" )

###################################
#   Basic environment checks      #
###################################

#### Checks for header files. ####

# ISO
AC_HEADER_STDC

# XPG4-UNIX
AC_CHECK_HEADERS([sys/poll.h])

# Other
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([byteswap.h])

#### Typdefs, structures, etc. ####

AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ssize_t, , [AC_DEFINE([ssize_t], [signed long],
    [Define ssize_t if it is not done by the standard libs.])])
AC_TYPE_OFF_T
AC_TYPE_UID_T

#### Check for libs ####

# ISO
AC_SEARCH_LIBS([pow], [m])

# POSIX
AC_SEARCH_LIBS([sched_setscheduler], [rt])

# Non-standard

#### Check for functions ####

# ISO
AC_CHECK_FUNCS([lrintf strtof])

# POSIX
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([gettimeofday nanosleep posix_memalign sigaction sleep sysconf pthread_yield])

# SUSv3
AC_CHECK_FUNCS([strerror_r])

# BSD
AC_CHECK_FUNCS([lstat])

# GNU
AC_CHECK_FUNCS([strndup])

#### POSIX threads ####

ACX_PTHREAD

#### Large File-Support (LFS) ####

AC_SYS_LARGEFILE

###################################
#      External libraries         #
###################################

#### pkg-config ####

PKG_PROG_PKG_CONFIG

#### ALSA support (optional) ####

AC_ARG_ENABLE([alsa],
    AS_HELP_STRING([--disable-alsa], [Disable optional ALSA support]),
        [
            case "${enableval}" in
                yes) alsa=yes ;;
                no) alsa=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-alsa) ;;
            esac
        ],
        [alsa=auto])

if test "x${alsa}" != xno ; then
    PKG_CHECK_MODULES(ALSA, [ alsa >= 1.0.0 ],
        [
            HAVE_ALSA=1
            AC_DEFINE([HAVE_ALSA], 1, [Have ALSA?])
        ],
        [
            HAVE_ALSA=0
            if test "x$alsa" = xyes ; then
                AC_MSG_ERROR([*** ALSA not found ***])
            fi
        ])
else
    HAVE_ALSA=0
fi

AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)

### OSS support (optional) ###

AC_ARG_ENABLE([oss],
    AS_HELP_STRING([--disable-oss], [Disable optional OSS support]),
        [
            case "${enableval}" in
                yes) oss=yes ;;
                no) oss=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-oss) ;;
            esac
        ],
        [oss=auto])

if test "x${oss}" != xno ; then
    AC_CHECK_HEADERS(soundcard.h sys/soundcard.h machine/soundcard.h)
    if test "${ac_cv_header_sys_soundcard_h}" = "yes" || \
	test "${ac_cv_header_soundcard_h}" = "yes" || \
	test "${ac_cv_header_machine_soundcard_h}" = "yes"; then
	HAVE_OSS=1
        AC_DEFINE([HAVE_OSS], 1, [Have OSS?])
    else
	HAVE_OSS=0
        if test "x$oss" = xyes ; then
            AC_MSG_ERROR([*** OSS not found ***])
        fi
    fi
else
    HAVE_OSS=0
fi

### PulseAudio (optional) ####

AC_ARG_ENABLE([pulse],
    AS_HELP_STRING([--disable-pulse], [Disable optional PulseAudio support]),
        [
            case "${enableval}" in
                yes) pulse=yes ;;
                no) pulse=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-pulse) ;;
            esac
        ],
        [pulse=auto])

if test "x${pulse}" != xno ; then

    if test -d ../pulseaudio ; then
        PULSE_CFLAGS='-I$(top_srcdir)/../pulseaudio/src'
        PULSE_LIBS='-L$(top_srcdir)/../pulseaudio/src/.libs -lpulse'
        HAVE_PULSE=1
        AC_DEFINE([HAVE_PULSE], 1, [Have PulseAudio?])
        echo "*** Found pulseaudio in ../pulseaudio, using that version ***"
    else
        PKG_CHECK_MODULES(PULSE, [ libpulse >= 0.9.11 ],
        [
            HAVE_PULSE=1
            AC_DEFINE([HAVE_PULSE], 1, [Have PulseAudio?])
        ],
        [
            HAVE_PULSE=0
            if test "x$pulse" = xyes ; then
                AC_MSG_ERROR([*** PulseAudio not found ***])
            fi
        ])
    fi
else
    HAVE_PULSE=0
fi

AC_SUBST(PULSE_CFLAGS)
AC_SUBST(PULSE_LIBS)

#### GStreamer support (optional) ####

AC_ARG_ENABLE([gstreamer],
    AS_HELP_STRING([--disable-gstreamer], [Disable optional GStreamer support]),
        [
            case "${enableval}" in
                yes) gstreamer=yes ;;
                no) gstreamer=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-gstreamer) ;;
            esac
        ],
        [gstreamer=auto])

if test "x${gstreamer}" != xno ; then
    PKG_CHECK_MODULES(GST, [ gstreamer-0.10 >= 0.10.15 ],
        [
            HAVE_GSTREAMER=1
            AC_DEFINE([HAVE_GSTREAMER], 1, [Have GStreamer?])
        ],
        [
            HAVE_GSTREAMER=0
            if test "x$gstreamer" = xyes ; then
                AC_MSG_ERROR([*** GStreamer not found ***])
            fi
        ])
else
    HAVE_GSTREAMER=0
fi

AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

### Null output (optional) ####

AC_ARG_ENABLE([null],
    AS_HELP_STRING([--disable-null], [Disable optional null output]),
        [
            case "${enableval}" in
                yes) null=yes ;;
                no) null=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-null) ;;
            esac
        ],
        [null=yes])

if test "x${null}" != xno ; then
    HAVE_NULL=1
    AC_DEFINE([HAVE_NULL], 1, [Have NULL output?])
else
    HAVE_NULL=0
fi

### GTK (optional) ####

AC_ARG_ENABLE([gtk],
    AS_HELP_STRING([--disable-gtk], [Disable optional GTK+ support]),
        [
            case "${enableval}" in
                yes) gtk=yes ;;
                no) gtk=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-gtk) ;;
            esac
        ],
        [gtk=auto])

AM_CONDITIONAL([GCONF_SCHEMAS_INSTALL], [false])
if test "x${gtk}" != xno ; then
    PKG_CHECK_MODULES(GTK, [ gtk+-2.0 >= 2.13.4 gthread-2.0 ],
        [
            HAVE_GTK=1
            AC_DEFINE([HAVE_GTK], 1, [Have GTK?])

            AC_PATH_PROG(GCONFTOOL, gconftool-2)
	    AM_GCONF_SOURCE_2

            GTK_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
            GTK_MODULES_DIR="${libdir}/gtk-2.0/modules"
        ],
        [
            HAVE_GTK=0
            if test "x$gtk" = xyes ; then
                AC_MSG_ERROR([*** GTK not found ***])
            fi
        ])
else
    HAVE_GTK=0
fi


AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_MODULES_DIR)
AC_SUBST(HAVE_GTK)
AM_CONDITIONAL([HAVE_GTK], [test "x$HAVE_GTK" = x1])

#### TDB cache support (optional) ####

AC_ARG_ENABLE([tdb],
    AS_HELP_STRING([--disable-tdb], [Disable optional tdb support]),
        [
            case "${enableval}" in
                yes) tdb=yes ;;
                no) tdb=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-tdb) ;;
            esac
        ],
        [tdb=auto])

if test "x${tdb}" != xno ; then
    PKG_CHECK_MODULES(TDB, [ tdb >= 1.1 ],
        [
            HAVE_TDB=1
            AC_DEFINE([HAVE_TDB], 1, [Have TDB?])
        ],
        [
            HAVE_TDB=0
            if test "x$tdb" = xyes ; then
                AC_MSG_ERROR([*** TDB not found ***])
            fi
        ])
else
    HAVE_TDB=0
fi

AC_SUBST(TDB_CFLAGS)
AC_SUBST(TDB_LIBS)

AC_SUBST(HAVE_TDB)
AM_CONDITIONAL([HAVE_TDB], [test "x$HAVE_TDB" = x1])

### Global cache support ###

# For now, we only support tdb based caching, hence we'll shortcut this here

HAVE_CACHE=$HAVE_TDB
AC_SUBST(HAVE_CACHE)
AM_CONDITIONAL([HAVE_CACHE], [test "x$HAVE_CACHE" = x1])

if test "x${HAVE_CACHE}" = x1 ; then
     AC_DEFINE([HAVE_CACHE], 1, [Do cacheing?])
fi

### LYNX documentation generation ###

AC_ARG_ENABLE([lynx],
        AS_HELP_STRING([--disable-lynx], [Turn off lynx usage for documentation generation]),
                [
                        case "${enableval}" in
                                yes) lynx=yes ;;
                                no) lynx=no ;;
                                *) AC_MSG_ERROR(bad value ${enableval} for --disable-lynx) ;;
                        esac
                ],
                [lynx=yes])

if test x$lynx = xyes ; then
   AC_CHECK_PROG(have_lynx, lynx, yes, no)

   if test "x$have_lynx" = xno ; then
     AC_MSG_WARN([*** lynx not found, plain text README will not be built ***])
   fi
fi

AM_CONDITIONAL([USE_LYNX], [test "x$have_lynx" = xyes])

### Vorbis (mandatory) ###

PKG_CHECK_MODULES(VORBIS, [ vorbisfile ])

### Chose builtin driver ###

AC_ARG_WITH([builtin],
            [AS_HELP_STRING([--with-builtin], [Choose builtin driver])],
            [],
            [with_builtin=dso])

HAVE_DSO=0
BUILTIN_DSO=0
BUILTIN_PULSE=0
BUILTIN_ALSA=0
BUILTIN_OSS=0
BUILTIN_GSTREAMER=0
BUILTIN_NULL=0

case "x$with_builtin" in
     xpulse)
        if test "x$HAVE_PULSE" != x1 ; then
                AC_MSG_ERROR([*** PulseAudio selected for builtin driver, but not enabled. ***])
        fi

        BUILTIN_PULSE=1
        HAVE_ALSA=0
	HAVE_OSS=0
	HAVE_GSTREAMER=0
        HAVE_NULL=0
     ;;

     xalsa)
        if test "x$HAVE_ALSA" != x1 ; then
                AC_MSG_ERROR([*** ALSA selected for builtin driver, but not enabled. ***])
        fi

        BUILTIN_ALSA=1
	HAVE_OSS=0
        HAVE_PULSE=0
	HAVE_GSTREAMER=0
        HAVE_NULL=0
     ;;

     xgstreamer)
        if test "x$HAVE_GSTREAMER" != x1 ; then
                AC_MSG_ERROR([*** GStremaer selected for builtin driver, but not enabled. ***])
        fi

        BUILTIN_GSTREAMER=1
        HAVE_ALSA=0
	HAVE_OSS=0
        HAVE_PULSE=0
        HAVE_NULL=0
     ;;

     xoss)
        if test "x$HAVE_OSS" != x1 ; then
	        AC_MSG_ERROR([*** OSS selected for builtin driver, but not enabled. ***])
	fi

	BUILTIN_OSS=1
	HAVE_ALSA=0
	HAVE_PULSE=0
	HAVE_GSTREAMER=0
	HAVE_NULL=0
     ;;

     xnull)
        if test "x$HAVE_NULL" != x1 ; then
                AC_MSG_ERROR([*** Null output selected for builtin driver, but not enabled. ***])
        fi

        BUILTIN_NULL=1
        HAVE_PULSE=0
        HAVE_ALSA=0
	HAVE_OSS=0
	HAVE_GSTREAMER=0
     ;;

     xdso)
        BUILTIN_DSO=1
        HAVE_DSO=1
        AC_DEFINE([HAVE_DSO], 1, [Have DSO?])
     ;;

     *)
        AC_MSG_ERROR([*** Unknown driver $with_builtin selected for builtin ***])
esac

if test "x$HAVE_PULSE" != x1 -a "x$HAVE_ALSA" != x1 -a "x$HAVE_OSS" != x1 -a "x$HAVE_GSTREAMER" != x1 -a "x$HAVE_NULL" != x1 ; then
   AC_MSG_ERROR([*** No backend enabled. ***])
fi

AC_SUBST(HAVE_DSO)
AC_SUBST(HAVE_PULSE)
AC_SUBST(HAVE_ALSA)
AC_SUBST(HAVE_OSS)
AC_SUBST(HAVE_GSTREAMER)
AC_SUBST(HAVE_NULL)
AC_SUBST(BUILTIN_DSO)
AC_SUBST(BUILTIN_PULSE)
AC_SUBST(BUILTIN_ALSA)
AC_SUBST(BUILTIN_OSS)
AC_SUBST(BUILTIN_GSTREAMER)
AC_SUBST(BUILTIN_NULL)
AM_CONDITIONAL([HAVE_PULSE], [test "x$HAVE_PULSE" = x1])
AM_CONDITIONAL([HAVE_ALSA], [test "x$HAVE_ALSA" = x1])
AM_CONDITIONAL([HAVE_OSS], [test "x$HAVE_OSS" = x1])
AM_CONDITIONAL([HAVE_GSTREAMER], [test "x$HAVE_GSTREAMER" = x1])
AM_CONDITIONAL([HAVE_NULL], [test "x$HAVE_NULL" = x1])
AM_CONDITIONAL([BUILTIN_DSO], [test "x$BUILTIN_DSO" = x1])
AM_CONDITIONAL([BUILTIN_PULSE], [test "x$BUILTIN_PULSE" = x1])
AM_CONDITIONAL([BUILTIN_ALSA], [test "x$BUILTIN_ALSA" = x1])
AM_CONDITIONAL([BUILTIN_OSS], [test "x$BUILTIN_OSS" = x1])
AM_CONDITIONAL([BUILTIN_GSTREAMER], [test "x$BUILTIN_GSTREAMER" = x1])
AM_CONDITIONAL([BUILTIN_NULL], [test "x$BUILTIN_NULL" = x1])

AC_SUBST([CA_MAJOR],[ca_major])
AC_SUBST([CA_MINOR],[ca_minor])

GTK_DOC_CHECK(1.9)

###################################
#            Output               #
###################################

AC_CONFIG_FILES([
Makefile
src/Makefile
libcanberra.pc
libcanberra-gtk.pc
src/canberra.h
gtkdoc/Makefile
doc/Makefile
doc/README.html
])
AC_OUTPUT

# ==========================================================================
ENABLE_BUILTIN_DSO=no
if test "x$BUILTIN_DSO" = "x1" ; then
   ENABLE_BUILTIN_DSO=yes
fi

ENABLE_PULSE=no
if test "x$HAVE_PULSE" = "x1" ; then
   ENABLE_PULSE=yes
fi
ENABLE_BUILTIN_PULSE=no
if test "x$BUILTIN_PULSE" = "x1" ; then
   ENABLE_BUILTIN_PULSE=yes
fi

ENABLE_ALSA=no
if test "x$HAVE_ALSA" = "x1" ; then
   ENABLE_ALSA=yes
fi
ENABLE_BUILTIN_ALSA=no
if test "x$BUILTIN_ALSA" = "x1" ; then
   ENABLE_BUILTIN_ALSA=yes
fi

ENABLE_OSS=no
if test "x$HAVE_OSS" = "x1" ; then
    ENABLE_OSS=yes
fi
ENABLE_BUILTIN_OSS=no
if test "x$BUILTIN_OSS" = "x1" ; then
    ENABLE_BUILTIN_OSS=yes
fi

ENABLE_GSTREAMER=no
if test "x$HAVE_GSTREAMER" = "x1" ; then
   ENABLE_GSTREAMER=yes
fi
ENABLE_BUILTIN_GSTREAMER=no
if test "x$BUILTIN_GSTREAMER" = "x1" ; then
   ENABLE_BUILTIN_GSTREAMER=yes
fi

ENABLE_NULL=no
if test "x$HAVE_NULL" = "x1" ; then
   ENABLE_NULL=yes
fi
ENABLE_BUILTIN_NULL=no
if test "x$BUILTIN_NULL" = "x1" ; then
   ENABLE_BUILTIN_NULL=yes
fi

ENABLE_GTK=no
if test "x$HAVE_GTK" = "x1" ; then
   ENABLE_GTK=yes
fi

ENABLE_TDB=no
if test "x$HAVE_TDB" = "x1" ; then
   ENABLE_TDB=yes
fi

ENABLE_CACHE=no
if test "x$HAVE_CACHE" = "x1" ; then
   ENABLE_CACHE=yes
fi

echo "
 ---{ $PACKAGE_NAME $VERSION }---

    prefix:                 ${prefix}
    sysconfdir:             ${sysconfdir}
    localstatedir:          ${localstatedir}
    Compiler:               ${CC}
    CFLAGS:                 ${CFLAGS}
    C++-Compiler:           ${CXX}
    CXXFLAGS:               ${CXXFLAGS}
    Builtin DSO:            ${ENABLE_BUILTIN_DSO}
    Enable PulseAudio:      ${ENABLE_PULSE}
    Builtin PulseAudio:     ${ENABLE_BUILTIN_PULSE}
    Enable ALSA:            ${ENABLE_ALSA}
    Builtin ALSA:           ${ENABLE_BUILTIN_ALSA}
    Enable OSS:             ${ENABLE_OSS}
    Builtin OSS:            ${ENABLE_BUILTIN_OSS}
    Enable GStreamer:       ${ENABLE_GSTREAMER}
    Builtin GStreamer:      ${ENABLE_BUILTIN_GSTREAMER}
    Enable Null Output:     ${ENABLE_NULL}
    Builtin Null Output:    ${ENABLE_BUILTIN_NULL}
    Enable tdb:             ${ENABLE_TDB}
    Enable lookup cache:    ${ENABLE_CACHE}
    Enable GTK+:            ${ENABLE_GTK}
    GTK Modules Directory:  ${GTK_MODULES_DIR}
"


if test "x$HAVE_OSS" = "x1" -a "x$HAVE_ALSA" = "x1" ; then

   echo "
WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!

You enabled the OSS driver although the ALSA driver is
available. Please note that if ALSA is available the OSS driver is a
very bad choice, since it currently doesn't support resampling or
converting sample types to the necessities of the hardware if the
hardware does not directly support the format/rate of the sound file
to play.

Packagers of Linux distributions! Please think twice if you package
the OSS driver! It is probably best to not to pacakge it at all -- at
least until the OSS driver learned sample type conversion and the most
basic resampling. Otherwise you might end up getting bug reports from
users misunderstanding the OSS vs. ALSA situation.

WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
"

fi
