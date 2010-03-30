/*-*- Mode: C; c-basic-offset: 8 -*-*/

/***
  This file is part of libcanberra.

  Copyright 2009 Lennart Poettering

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
#include <sys/types.h>

#include "fork-detect.h"

int ca_detect_fork(void) {
        static volatile pid_t pid = (pid_t) -1;
        pid_t v, we;

        /* Some really stupid applications (Hey, vim, that means you!)
         * love to fork after initializing gtk/libcanberra. This is really
         * bad style. We however have to deal with this cleanly, so we try
         * to detect the forks making sure all our calls fail cleanly
         * after the fork. */

        /* Ideally we'd use atomic operations here, but we don't have them
         * and this is not exactly crucial, so we don't care */

        v = pid;
        we = getpid();

        if (v == we || v == (pid_t) -1) {
                pid = we;
                return 0;
        }

        return 1;
}
