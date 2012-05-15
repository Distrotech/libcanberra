#!/bin/bash

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

if [ -f .git/hooks/pre-commit.sample -a ! -f .git/hooks/pre-commit ] ; then
        cp -p .git/hooks/pre-commit.sample .git/hooks/pre-commit && \
        chmod +x .git/hooks/pre-commit && \
        echo "Activated pre-commit hook."
fi

GTKDOCIZE=$(which gtkdocize 2>/dev/null)
if test -z $GTKDOCIZE; then
        echo "You don't have gtk-doc installed, and thus won't be able to generate the documentation."
        echo 'EXTRA_DIST =' > gtkdoc/gtk-doc.make
else
        gtkdocize --docdir gtkdoc/
        gtkdocargs=--enable-gtk-doc
fi

autoreconf --force --install --symlink

libdir() {
        echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

args="\
--sysconfdir=/etc \
--localstatedir=/var \
--libdir=$(libdir /usr/lib) \
--libexecdir=/usr/lib \
$gtkdocargs"

if [ "x$1" == "xc" ]; then
        ./configure CFLAGS='-g -O0 -Wp,-U_FORTIFY_SOURCE' $args
        make clean
else
        echo
        echo "----------------------------------------------------------------"
        echo "Initialized build system. For a common configuration please run:"
        echo "----------------------------------------------------------------"
        echo
        echo "./configure CFLAGS='-g -O0 -Wp,-U_FORTIFY_SOURCE' $args"
        echo
fi
