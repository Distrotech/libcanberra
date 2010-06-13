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

noinst_DATA = README.html README
EXTRA_DIST = $(noinst_DATA) style.css README.html.in

MAINTAINERCLEANFILES = README.html
CLEANFILES =

if USE_LYNX
README: README.html
	$(AM_V_GEN)lynx --dump $^ | sed 's,file://localhost/.*/doc/README.html,README,' > $@

CLEANFILES += README
endif

tidy: README.html
	tidy -qe < README.html ; true

.PHONY: tidy
