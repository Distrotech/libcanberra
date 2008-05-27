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

#include <stdio.h>
#include <stdarg.h>

#include "malloc.h"
#include "macro.h"

char *ca_sprintf_malloc(const char *format, ...) {
    int  size = 100;
    char *c = NULL;

    ca_assert(format);

    for(;;) {
        int r;
        va_list ap;

        ca_free(c);

        if (!(c = ca_new(char, size)))
            return NULL;

        va_start(ap, format);
        r = vsnprintf(c, size, format, ap);
        va_end(ap);

        c[size-1] = 0;

        if (r > -1 && r < size)
            return c;

        if (r > -1)    /* glibc 2.1 */
            size = r+1;
        else           /* glibc 2.0 */
            size *= 2;
    }
}
