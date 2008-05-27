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

#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#include "canberra.h"
#include "read-vorbis.h"
#include "macro.h"
#include "malloc.h"

#define FILE_SIZE_MAX (64U*1024U*1024U)

struct ca_vorbis {
    OggVorbis_File ovf;
};

static int convert_error(int or) {
    switch (or) {
        case OV_ENOSEEK:
        case OV_EBADPACKET:
        case OV_EBADLINK:
        case OV_EFAULT:
        case OV_EREAD:
        case OV_HOLE:
            return CA_ERROR_IO;

        case OV_EIMPL:
        case OV_EVERSION:
        case OV_ENOTAUDIO:
            return CA_ERROR_NOTSUPPORTED;

        case OV_ENOTVORBIS:
        case OV_EBADHEADER:
        case OV_EOF:
            return CA_ERROR_CORRUPT;

        case OV_EINVAL:
            return CA_ERROR_INVALID;

        default:
            return CA_ERROR_IO;
    }
}

int ca_vorbis_open(ca_vorbis **_v, FILE *f)  {
    int ret, or;
    ca_vorbis *v;
    int64_t n;

    ca_return_val_if_fail(_v, CA_ERROR_INVALID);
    ca_return_val_if_fail(f, CA_ERROR_INVALID);

    if (!(v = ca_new(ca_vorbis, 1)))
        return CA_ERROR_OOM;

    if ((or = ov_open(f, &v->ovf, NULL, 0)) < 0) {
        ret = convert_error(or);
        goto fail;
    }

    if ((n = ov_pcm_total(&v->ovf, -1)) < 0) {
        ret = convert_error(or);
        ov_clear(&v->ovf);
        goto fail;
    }

    if (n * sizeof(int16_t) > FILE_SIZE_MAX) {
        ret = CA_ERROR_TOOBIG;
        ov_clear(&v->ovf);
        goto fail;
    }

    *_v = v;

    return CA_SUCCESS;

fail:

    ca_free(v);
    return ret;
}

void ca_vorbis_close(ca_vorbis *v) {
    ca_assert(v);

    ov_clear(&v->ovf);
    ca_free(v);
}

unsigned ca_vorbis_get_nchannels(ca_vorbis *v) {
    const vorbis_info *vi;
    ca_assert(v);

    ca_assert_se(vi = ov_info(&v->ovf, -1));

    return vi->channels;
}

unsigned ca_vorbis_get_rate(ca_vorbis *v) {
    const vorbis_info *vi;
    ca_assert(v);

    ca_assert_se(vi = ov_info(&v->ovf, -1));

    return (unsigned) vi->rate;
}

int ca_vorbis_read_s16ne(ca_vorbis *v, int16_t *d, unsigned *n){
    long r;
    int section;

    ca_return_val_if_fail(v, CA_ERROR_INVALID);
    ca_return_val_if_fail(d, CA_ERROR_INVALID);
    ca_return_val_if_fail(n, CA_ERROR_INVALID);
    ca_return_val_if_fail(*n > 0, CA_ERROR_INVALID);

    r = ov_read(&v->ovf, (char*) d, *n * sizeof(float),
#ifdef WORDS_BIGENDIAN
                1,
#else
                0,
#endif
                2, 1, &section);

    if (r < 0)
        return convert_error(r);

    /* We only read the first section */
    if (section != 0)
        return 0;

    *n = (unsigned) r;
    return CA_SUCCESS;
}
