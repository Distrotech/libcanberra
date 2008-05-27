/* $Id$ */

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
  License along with libcanberra. If not, If not, see
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

int main(int argc, char *argv[]) {
    ca_context *c;

    setlocale(LC_ALL, "");

    ca_context_create(&c);

    /* Initialize a few meta variables for the following play()
     * calls. They stay valid until they are overwritten with
     * ca_context_set() again. */
    ca_context_change_props(c,
                            CA_PROP_APPLICATION_NAME, "An example",
                            CA_PROP_APPLICATION_ID, "org.freedesktop.libcanberra.Test",
                            CA_PROP_WINDOW_X11_DISPLAY, getenv("DISPLAY"),
                            NULL);

    /* .. */

    ca_context_open(c);


    fprintf(stderr, "Play ...\n");

    /* Signal a sound event. The meta data passed here overwrites the
     * data set in any previous ca_context_set() calls. */
/*     ca_context_play(c, 0, */
/*                     CA_PROP_EVENT_ID, "desktop-logout", */
/*                     CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/bar.wav", */
/*                     CA_PROP_MEDIA_NAME, "User has logged off from session", */
/*                     CA_PROP_MEDIA_LANGUAGE, "en_EN", */
/*                     NULL); */

    /* .. */


    ca_context_play(c, 1,
                    CA_PROP_EVENT_ID, "email-message-new",
                    CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/uxknkurz.wav",
                    CA_PROP_MEDIA_NAME, "New email received",
                    CA_PROP_CANBERRA_CACHE_CONTROL, "permanent",
                    NULL);
    /* .. */

    fprintf(stderr, "Sleep half a second ...\n");
    usleep(500000);


    fprintf(stderr, "Cancel ...\n");
    /* Stop one sounds */
    ca_context_cancel(c, 0);

    fprintf(stderr, "Sleep 2s ...\n");
    sleep(2);

    /* .. */

    ca_context_destroy(c);

    return 0;
}
