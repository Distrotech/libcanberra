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

dist_doc_DATA = README

EXTRA_DIST = bootstrap.sh autogen.sh LGPL
SUBDIRS = src gtkdoc libltdl doc

MAINTAINERCLEANFILES = README
noinst_DATA = README

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcanberra.pc

if HAVE_GTK
pkgconfig_DATA += libcanberra-gtk.pc
endif

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
ACLOCAL_AMFLAGS = -I m4

README:
	rm -f README
	$(MAKE) -C doc README
	cd $(srcdir) && ln -s doc/README README

homepage: all dist
	test -d $$HOME/homepage/private
	mkdir -p $$HOME/homepage/private/projects/libcanberra $$HOME/homepage/private/projects/libcanberra/gtkdoc
	cp libcanberra-@PACKAGE_VERSION@.tar.gz $$HOME/homepage/private/projects/libcanberra
	cp doc/README.html doc/style.css $$HOME/homepage/private/projects/libcanberra
	cp -a gtkdoc/html/* $$HOME/homepage/private/projects/libcanberra/gtkdoc
	ln -sf README.html $$HOME/homepage/private/projects/libcanberra/index.html

.PHONY: homepage
