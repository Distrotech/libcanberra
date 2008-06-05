# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

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

AC_PREREQ(2.57)

AC_INIT([libcanberra], 0.1, [mzyvopnaoreen (at) 0pointer (dot) de])
AC_CONFIG_SRCDIR([src/common.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall])

AC_SUBST(PACKAGE_URL, [https://tango.0pointer.de/mailman/listinfo/libcanberra-discuss/])

AC_SUBST(LIBCANBERRA_VERSION_INFO, [0:0:0])

AC_CANONICAL_HOST

if type -p stow > /dev/null && test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION} ***])
   ac_default_prefix="/usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION}"
fi

#### Checks for programs. ####

# CC

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
AC_GNU_SOURCE

# C++

AC_PROG_CXX

# GCC flags

test_gcc_flag() {
    AC_LANG_CONFTEST([int main(int argc, char*argv[]) {}])
    $CC -c conftest.c $CFLAGS -o conftest.o > /dev/null 2> /dev/null
    ret=$?
    rm -f conftest.o
    return $ret
}

# If using GCC specify some additional parameters
if test "x$GCC" = "xyes" ; then

    # We use gnu99 instead of c99 because many have interpreted the standard
    # in a way that int64_t isn't defined on non-64 bit platforms.
    DESIRED_FLAGS="-std=gnu99 -Wall -W -Wextra -pedantic -pipe -Wformat -Wold-style-definition -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wmissing-noreturn -Wshadow -Wendif-labels -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wno-unused-parameter -ffast-math"

    for flag in $DESIRED_FLAGS ; do
        AC_MSG_CHECKING([whether $CC accepts $flag])
        if test_gcc_flag $flag ; then
           CFLAGS="$CFLAGS $flag"
           CXXFLAGS="$CXXFLAGS $flag"
           AC_MSG_RESULT([yes])
        else
           AC_MSG_RESULT([no])
        fi
    done
fi

#### libtool stuff ####

AC_LTDL_ENABLE_INSTALL
AC_LIBLTDL_INSTALLABLE
AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_SUBST(LTDLINCL)
AC_SUBST(LIBLTDL)
AC_CONFIG_SUBDIRS(libltdl)

#### Determine build environment ####

os_is_win32=0

case "$host_os" in
     mingw*)
        AC_DEFINE([OS_IS_WIN32], 1, [Build target is Windows.])
        os_is_win32=1
     ;;
esac

AM_CONDITIONAL(OS_IS_WIN32, test "x$os_is_win32" = "x1")

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
AC_TYPE_SIGNAL
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

#### POSIX threads ####

ACX_PTHREAD

#### Large File-Support (LFS) ####

AC_SYS_LARGEFILE

#### [lib]iconv ####

AM_ICONV

###################################
#      External libraries         #
###################################

#### pkg-config ####

# Check for pkg-config manually first, as if its not installed the
# PKG_PROG_PKG_CONFIG macro won't be defined.
AC_CHECK_PROG(have_pkg_config, pkg-config, yes, no)

if test x"$have_pkg_config" = "xno"; then
    AC_MSG_ERROR(pkg-config is required to install this program)
fi

PKG_PROG_PKG_CONFIG

#### ALSA support (optional) ####

AC_ARG_ENABLE([alsa],
    AC_HELP_STRING([--disable-alsa], [Disable optional ALSA support]),
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

### PulseAudio (optional) ####

AC_ARG_ENABLE([pulse],
    AC_HELP_STRING([--disable-pulse], [Disable optional PulseAudio support]),
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
        PULSE_LIBS='-L$(top_srcdir)/../pulseaudio/src/.libs -lpulse -lpulse-mainloop-glib'
        HAVE_PULSE=1
        echo "*** Found pulseaudio in ../pulseaudio, using that version ***"
    else
        PKG_CHECK_MODULES(PULSE, [ libpulse >= 0.9.11 libpulse-mainloop-glib >= 0.9.11 ],
        [
            HAVE_PULSE=1,
            AC_DEFINE([HAVE_ALSA], 1, [Have ALSA?])
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

### GTK (optional) ####

AC_ARG_ENABLE([gtk],
    AC_HELP_STRING([--disable-gtk], [Disable optional GTK+ support]),
        [
            case "${enableval}" in
                yes) gtk=yes ;;
                no) gtk=no ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --disable-gtk) ;;
            esac
        ],
        [gtk=auto])

if test "x${gtk}" != xno ; then
    PKG_CHECK_MODULES(GTK, [ gtk+-2.0 ],
        [
            HAVE_GTK=1
            AC_DEFINE([HAVE_GTK], 1, [Have GTK?])

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

case "x$with_builtin" in
     xpulse)
        if test "x$HAVE_PULSE" != x1 ; then
                AC_MSG_ERROR([*** PulseAudio selected for builtin driver, but not enabled. ***])
        fi

        BUILTIN_PULSE=1
        HAVE_ALSA=0
     ;;

     xalsa)
        if test "x$HAVE_ALSA" != x1 ; then
                AC_MSG_ERROR([*** ALSA selected for builtin driver, but not enabled. ***])
        fi

        BUILTIN_ALSA=1
        HAVE_PULSE=0
     ;;

     xdso)
        BUILTIN_DSO=1
        HAVE_DSO=1
        AC_DEFINE([HAVE_DSO], 1, [Have DSO?])
     ;;

     *)
        AC_MSG_ERROR([*** Unknown driver $with_builtin selected for builtin ***])
esac

if test "x$HAVE_PULSE" != x1 -a "x$HAVE_ALSA" != x1 ; then
   AC_MSG_ERROR([*** No backend enabled. ***])
fi

AC_SUBST(HAVE_DSO)
AC_SUBST(HAVE_PULSE)
AC_SUBST(HAVE_ALSA)
AC_SUBST(BUILTIN_DSO)
AC_SUBST(BUILTIN_ALSA)
AC_SUBST(BUILTIN_PULSE)
AM_CONDITIONAL([HAVE_PULSE], [test "x$HAVE_PULSE" = x1])
AM_CONDITIONAL([HAVE_ALSA], [test "x$HAVE_ALSA" = x1])
AM_CONDITIONAL([BUILTIN_DSO], [test "x$BUILTIN_DSO" = x1])
AM_CONDITIONAL([BUILTIN_PULSE], [test "x$BUILTIN_PULSE" = x1])
AM_CONDITIONAL([BUILTIN_ALSA], [test "x$BUILTIN_ALSA" = x1])

###################################
#            Output               #
###################################

AC_CONFIG_FILES([
Makefile
src/Makefile
libcanberra.pc
libcanberra-gtk.pc
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

ENABLE_GTK=no
if test "x$HAVE_GTK" = "x1" ; then
   ENABLE_GTK=yes
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
    Enable GTK+:            ${ENABLE_GTK}
    GTK Modules Directory:  ${GTK_MODULES_DIR}
"
