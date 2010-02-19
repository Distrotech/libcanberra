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

#include <stdlib.h>

#include "canberra.h"
#include "driver-order.h"

const char* const ca_driver_order[] = {
#ifdef HAVE_PULSE
        "pulse",
#endif
#ifdef HAVE_ALSA
        "alsa",
#endif
#ifdef HAVE_OSS
        "oss",
#endif
#ifdef HAVE_GSTREAMER
        "gstreamer",
#endif
        /* ... */
        NULL
};
