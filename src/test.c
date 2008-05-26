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

#include <unistd.h>

#include "canberra.h"

int main(int argc, char *argv[]) {
    ca_context_t *c;

    int id = 4711;

    ca_context_new(&c);

    /* Initialize a few meta variables for the following play()
     * calls. They stay valid until they are overwritten with
     * ca_context_set() again. */
    ca_context_change_props(c,
                    CA_PROP_APPLICATION_NAME, "An example",
                    CA_PROP_APPLICATION_ID, "org.freedesktop.libcanberra.Test",
                    CA_PROP_MEDIA_LANGUAGE, "de_DE",
                    CA_PROP_EVENT_X11_DISPLAY, getenv("DISPLAY"),
                     NULL);

    /* .. */

    ca_context_open(c);


    /* Signal a sound event. The meta data passed here overwrites the
     * data set in any previous ca_context_set() calls. */
    ca_context_play(c, id,
                     CA_PROP_EVENT_ID, "click-event",
                     CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/foo.wav",
                     CA_PROP_MEDIA_NAME, "Button has been clicked",
                     CA_PROP_MEDIA_ICON_NAME, "clicked",
                     NULL);

    /* .. */

    ca_context_play(c, id,
                    CA_PROP_EVENT_ID, "logout",
                    CA_PROP_MEDIA_FILENAME, "/usr/share/sounds/bar.wav",
                    CA_PROP_MEDIA_NAME, "User has logged of from session",
                    CA_PROP_MEDIA_LANGUAGE, "en_EN",
                    NULL);

    /* .. */

    sleep(1);

    /* Stops both sounds */
    ca_context_cancel(c, id);

    /* .. */

    ca_context_destroy(c);

    return 0;
}
