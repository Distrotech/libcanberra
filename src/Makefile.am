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

gtkmoduledir = @GTK_MODULES_DIR@
gtk3moduledir = @GTK3_MODULES_DIR@

plugindir = $(libdir)/libcanberra-@PACKAGE_VERSION@

gnomeshutdowndir = $(datadir)/gnome/shutdown

gnomeautostartdir = $(datadir)/gnome/autostart

gdmautostartdir = $(datadir)/gdm/autostart/LoginWindow

gsdgtkmodulesdir = $(libdir)/gnome-settings-daemon-3.0/gtk-modules

AM_CFLAGS = $(WARNINGFLAGS) $(PTHREAD_CFLAGS) -DCA_PLUGIN_PATH=\"$(plugindir)\"
#AM_CFLAGS = $(WARNINGFLAGS) $(PTHREAD_CFLAGS) -DCA_PLUGIN_PATH=\"/home/lennart/projects/libcanberra/src/.libs\"
AM_CXXFLAGS = $(WARNINGFLAGS) $(PTHREAD_CFLAGS)
AM_LDADD = $(PTHREAD_LIBS)
AM_LDFLAGS = $(GCLDFLAGS)

EXTRA_DIST = \
	map-file

lib_LTLIBRARIES = \
        libcanberra.la

include_HEADERS = \
	canberra.h

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
	macro.h macro.c \
	malloc.c malloc.h \
	fork-detect.c fork-detect.h
libcanberra_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VORBIS_CFLAGS)
libcanberra_la_LIBADD = \
	$(VORBIS_LIBS)
libcanberra_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBCANBERRA_VERSION_INFO)

if USE_VERSION_SCRIPT
libcanberra_la_LDFLAGS += -Wl,-version-script=$(srcdir)/map-file
endif

if HAVE_CACHE

libcanberra_la_SOURCES += \
	cache.c cache.h
libcanberra_la_CFLAGS += \
	$(TDB_CLFAGS) \
	-DCA_MACHINE_ID=\"$(localstatedir)/lib/dbus/machine-id\"
libcanberra_la_LIBADD += \
	$(TDB_LIBS)

endif

plugin_LTLIBRARIES =

if BUILTIN_DSO

libcanberra_la_SOURCES += \
	dso.c \
	driver-order.c driver-order.h
libcanberra_la_LIBADD += \
	$(LIBLTDL)

plugin_LTLIBRARIES += \
	libcanberra-multi.la

libcanberra_multi_la_SOURCES = \
	multi.c
libcanberra_multi_la_CFLAGS = \
	 -Ddriver_open=multi_driver_open \
	 -Ddriver_destroy=multi_driver_destroy \
	 -Ddriver_change_device=multi_driver_change_device \
	 -Ddriver_change_props=multi_driver_change_props \
	 -Ddriver_play=multi_driver_play \
	 -Ddriver_cancel=multi_driver_cancel \
	 -Ddriver_cache=multi_driver_cache
libcanberra_multi_la_LIBADD = \
	libcanberra.la
libcanberra_multi_la_LDFLAGS = \
	-avoid-version -module -export-dynamic
endif

if HAVE_PULSE
if BUILTIN_PULSE

libcanberra_la_SOURCES += \
	pulse.c
libcanberra_la_CFLAGS += \
	$(PULSE_CFLAGS)
libcanberra_la_LIBADD += \
	$(PULSE_LIBS)

else

plugin_LTLIBRARIES += \
	libcanberra-pulse.la

libcanberra_pulse_la_SOURCES = \
	pulse.c
libcanberra_pulse_la_CFLAGS = \
	$(PULSE_CFLAGS) \
	 -Ddriver_open=pulse_driver_open \
	 -Ddriver_destroy=pulse_driver_destroy \
	 -Ddriver_change_device=pulse_driver_change_device \
	 -Ddriver_change_props=pulse_driver_change_props \
	 -Ddriver_play=pulse_driver_play \
	 -Ddriver_cancel=pulse_driver_cancel \
	 -Ddriver_cache=pulse_driver_cache
libcanberra_pulse_la_LIBADD = \
	$(PULSE_LIBS) \
	libcanberra.la
libcanberra_pulse_la_LDFLAGS = \
	-avoid-version -module -export-dynamic

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

plugin_LTLIBRARIES += \
	libcanberra-alsa.la

libcanberra_alsa_la_SOURCES = \
	alsa.c
libcanberra_alsa_la_CFLAGS = \
	$(ALSA_CFLAGS) \
	 -Ddriver_open=alsa_driver_open \
	 -Ddriver_destroy=alsa_driver_destroy \
	 -Ddriver_change_device=alsa_driver_change_device \
	 -Ddriver_change_props=alsa_driver_change_props \
	 -Ddriver_play=alsa_driver_play \
	 -Ddriver_cancel=alsa_driver_cancel \
	 -Ddriver_cache=alsa_driver_cache
libcanberra_alsa_la_LIBADD = \
	$(ALSA_LIBS) \
	libcanberra.la
libcanberra_alsa_la_LDFLAGS = \
	-avoid-version -module -export-dynamic

endif
endif

if HAVE_OSS
if BUILTIN_OSS

libcanberra_la_SOURCES += \
	oss.c

else

plugin_LTLIBRARIES += \
	libcanberra-oss.la

libcanberra_oss_la_SOURCES = \
	oss.c
libcanberra_oss_la_CFLAGS = \
	 -Ddriver_open=oss_driver_open \
	 -Ddriver_destroy=oss_driver_destroy \
	 -Ddriver_change_device=oss_driver_change_device \
	 -Ddriver_change_props=oss_driver_change_props \
	 -Ddriver_play=oss_driver_play \
	 -Ddriver_cancel=oss_driver_cancel \
	 -Ddriver_cache=oss_driver_cache
libcanberra_oss_la_LIBADD = \
	libcanberra.la
libcanberra_oss_la_LDFLAGS = \
	-avoid-version -module -export-dynamic
endif
endif

if HAVE_GSTREAMER
if BUILTIN_GSTREAMER

libcanberra_la_SOURCES += \
	gstreamer.c
libcanberra_la_CFLAGS += \
	$(GST_CFLAGS)
libcanberra_la_LIBADD += \
	 $(GST_LIBS)

else

plugin_LTLIBRARIES += \
	libcanberra-gstreamer.la

libcanberra_gstreamer_la_SOURCES = \
	gstreamer.c
libcanberra_gstreamer_la_CFLAGS = \
	$(GST_CFLAGS) \
	 -Ddriver_open=gstreamer_driver_open \
	 -Ddriver_destroy=gstreamer_driver_destroy \
	 -Ddriver_change_device=gstreamer_driver_change_device \
	 -Ddriver_change_props=gstreamer_driver_change_props \
	 -Ddriver_play=gstreamer_driver_play \
	 -Ddriver_cancel=gstreamer_driver_cancel \
	 -Ddriver_cache=gstreamer_driver_cache
libcanberra_gstreamer_la_LIBADD = \
	$(GST_LIBS) \
	libcanberra.la
libcanberra_gstreamer_la_LDFLAGS = \
	-avoid-version -module -export-dynamic

endif
endif

if HAVE_NULL
if BUILTIN_NULL

libcanberra_la_SOURCES += \
	null.c

else

plugin_LTLIBRARIES += \
	libcanberra-null.la

libcanberra_null_la_SOURCES = \
	null.c
libcanberra_null_la_CFLAGS = \
	 -Ddriver_open=null_driver_open \
	 -Ddriver_destroy=null_driver_destroy \
	 -Ddriver_change_device=null_driver_change_device \
	 -Ddriver_change_props=null_driver_change_props \
	 -Ddriver_play=null_driver_play \
	 -Ddriver_cancel=null_driver_cancel \
	 -Ddriver_cache=null_driver_cache
libcanberra_null_la_LIBADD = \
	libcanberra.la
libcanberra_null_la_LDFLAGS = \
	-avoid-version -module -export-dynamic
endif
endif

bin_PROGRAMS =
CLEANFILES =

if HAVE_UDEV
if HAVE_ALSA

bin_PROGRAMS += \
	canberra-boot

canberra_boot_SOURCES = \
	canberra-boot.c

canberra_boot_LDADD = \
	$(UDEV_LIBS) \
	libcanberra.la

canberra_boot_CFLAGS = \
	$(UDEV_CFLAGS)

if HAVE_SYSTEMD

systemdsystemunit_DATA = \
	canberra-system-bootup.service \
	canberra-system-shutdown.service \
	canberra-system-shutdown-reboot.service

EXTRA_DIST += \
	canberra-system-bootup.service.in \
	canberra-system-shutdown.service.in \
	canberra-system-shutdown-reboot.service.in

%.service: %.service.in
	$(AM_V_GEN)sed -e 's,@bindir\@,$(bindir),g' $< > $@

CLEANFILES += \
	$(systemdsystemunit_DATA)

endif
endif
endif

if HAVE_GTK_ANY

bin_PROGRAMS += \
	canberra-gtk-play

include_HEADERS += \
	canberra-gtk.h

endif

if HAVE_GTK3

lib_LTLIBRARIES += \
	libcanberra-gtk3.la

gtk3module_LTLIBRARIES = \
	libcanberra-gtk3-module.la

libcanberra_gtk3_la_SOURCES = \
	canberra-gtk.h \
	canberra-gtk.c
libcanberra_gtk3_la_CFLAGS = \
	$(GTK3_CFLAGS)
libcanberra_gtk3_la_LIBADD = \
	$(GTK3_LIBS) \
	libcanberra.la
libcanberra_gtk3_la_LDFLAGS = \
	-export-dynamic -version-info $(LIBCANBERRA_GTK_VERSION_INFO)

libcanberra_gtk3_module_la_SOURCES = \
	canberra-gtk-module.c
libcanberra_gtk3_module_la_CFLAGS = \
	$(GTK3_CFLAGS)
libcanberra_gtk3_module_la_LIBADD = \
	$(GTK3_LIBS) \
	libcanberra.la \
	libcanberra-gtk3.la
libcanberra_gtk3_module_la_LDFLAGS = \
	-avoid-version -module -export-dynamic

install-exec-hook:
	$(MKDIR_P) -m 755 $(DESTDIR)$(gtk3moduledir)
	( cd $(DESTDIR)$(gtk3moduledir) && \
		rm -f libcanberra-gtk-module.so && \
	        $(LN_S) libcanberra-gtk3-module.so libcanberra-gtk-module.so )

endif

if HAVE_GTK

lib_LTLIBRARIES += \
	libcanberra-gtk.la

gtkmodule_LTLIBRARIES = \
	libcanberra-gtk-module.la

libcanberra_gtk_la_SOURCES = \
	canberra-gtk.h \
	canberra-gtk.c
libcanberra_gtk_la_CFLAGS = \
	$(GTK_CFLAGS)
libcanberra_gtk_la_LIBADD = \
	$(GTK_LIBS) \
	libcanberra.la
libcanberra_gtk_la_LDFLAGS = \
	-export-dynamic -version-info $(LIBCANBERRA_GTK_VERSION_INFO)

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

if HAVE_GTK3

canberra_gtk_play_LDADD = \
	$(GTK3_LIBS) \
	libcanberra.la \
	libcanberra-gtk3.la
canberra_gtk_play_CFLAGS = \
	$(GTK3_CFLAGS)

else
if HAVE_GTK

canberra_gtk_play_LDADD = \
	$(GTK_LIBS) \
	libcanberra.la \
	libcanberra-gtk.la
canberra_gtk_play_CFLAGS = \
	$(GTK_CFLAGS)

endif
endif

if HAVE_GTK_ANY

dist_gsdgtkmodules_DATA = \
	canberra-gtk-module.desktop

canberra_gtk_play_SOURCES = \
	canberra-gtk-play.c

EXTRA_DIST += \
	libcanberra-login-sound.desktop.in \
	libcanberra-ready-sound.desktop.in \
	libcanberra-logout-sound.sh.in

gnomeautostart_DATA = \
	libcanberra-login-sound.desktop

gdmautostart_DATA = \
	libcanberra-ready-sound.desktop

gnomeshutdown_SCRIPTS = \
	libcanberra-logout-sound.sh

CLEANFILES += \
	libcanberra-login-sound.desktop \
	libcanberra-ready-sound.desktop \
	libcanberra-logout-sound.sh

libcanberra-logout-sound.sh: libcanberra-logout-sound.sh.in Makefile
	$(AM_V_GEN)sed -e 's,@bindir\@,$(bindir),g' < $< > $@ && \
		chmod +x $@

libcanberra-login-sound.desktop: libcanberra-login-sound.desktop.in Makefile
	$(AM_V_GEN)sed -e 's,@bindir\@,$(bindir),g' < $< > $@

libcanberra-ready-sound.desktop: libcanberra-ready-sound.desktop.in Makefile
	$(AM_V_GEN)sed -e 's,@bindir\@,$(bindir),g' < $< > $@

endif

test_canberra_SOURCES = \
        test-canberra.c
test_canberra_LDADD = \
        $(AM_LDADD) \
        libcanberra.la
