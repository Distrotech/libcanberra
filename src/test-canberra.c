/*-*- Mode: C; c-basic-offset: 8 -*-*/

/***
  This file is part of libcanberra.

  Copyright 2008 Lennart Poettering

  libcanberra is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 2.1 of the
  License, or (at your option) any later version.

  libcanberra is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with libcanberra. If not, see
  <http://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <stdio.h>

#include "canberra.h"

static void callback(ca_context *c, uint32_t id, int error, void *userdata) {
        fprintf(stderr, "callback called for id %u, error '%s', userdata=%p\n", id, ca_strerror(error), userdata);
}

int main(int argc, char *argv[]) {
        ca_context *c;
        ca_proplist *p;
        int ret;

        setlocale(LC_ALL, "");

        ret = ca_context_create(&c);
        fprintf(stderr, "create: %s\n", ca_strerror(ret));

        /* Initialize a few meta variables for the following play()
         * calls. They stay valid until they are overwritten with
         * ca_context_change_props() again. */
        ret = ca_context_change_props(c,
                                      CA_PROP_APPLICATION_NAME, "An example",
                                      CA_PROP_APPLICATION_ID, "org.freedesktop.libcanberra.Test",
                                      CA_PROP_WINDOW_X11_SCREEN, getenv("DISPLAY"),
                                      NULL);
        fprintf(stderr, "change_props: %s\n", ca_strerror(ret));

        ret = ca_context_open(c);
        fprintf(stderr, "open: %s\n", ca_strerror(ret));

        /* Now trigger a sound event, the quick version */
        ret = ca_context_play(c, 0,
                              CA_PROP_EVENT_ID, "desktop-login",
                              CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/bar.wav",
                              CA_PROP_MEDIA_NAME, "User has logged off from session",
                              CA_PROP_MEDIA_LANGUAGE, "en_EN",
                              CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                              NULL);
        fprintf(stderr, "play: %s\n", ca_strerror(ret));

        /* Now trigger a sound event, the complex version */
        ca_proplist_create(&p);
        ca_proplist_sets(p, CA_PROP_EVENT_ID, "desktop-logout");
        ca_proplist_sets(p, CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/uxknkurz.wav");
        ca_proplist_sets(p, CA_PROP_MEDIA_NAME, "New email received");
        ca_proplist_setf(p, "test.foo", "%u", 4711);
        ret = ca_context_play_full(c, 1, p, callback, (void*) 0x4711);
        ca_proplist_destroy(p);
        fprintf(stderr, "play_full: %s\n", ca_strerror(ret));

        /* Now trigger a sound event, by filename */
        ret = ca_context_play(c, 2,
                              CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/freedesktop/stereo/audio-channel-front-left.ogg",
                              CA_PROP_MEDIA_NAME, "Front Left",
                              CA_PROP_MEDIA_LANGUAGE, "en_EN",
                              NULL);
        fprintf(stderr, "play (by filename): %s\n", ca_strerror(ret));

        fprintf(stderr, "Sleep half a second ...\n");
        usleep(500000);

        /* Stop one sound */
/*     ret = ca_context_cancel(c, 0); */
/*     fprintf(stderr, "cancel: %s\n", ca_strerror(ret)); */

        fprintf(stderr, "Sleep 2s ...\n");
        sleep(2);

        /* .. */

        ret = ca_context_destroy(c);
        fprintf(stderr, "destroy: %s\n", ca_strerror(ret));

        return 0;
}
