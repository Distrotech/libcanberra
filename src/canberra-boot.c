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

#include <stdio.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <canberra.h>
#include <libudev.h>

#include "macro.h"

static char *find_device(void) {
        struct udev *udev = NULL;
        struct udev_enumerate *udev_enum = NULL;
        struct udev_list_entry *i, *first;
        int internal_device = -1, pci_device = -1, other_device = -1;
        char *s = NULL;

        if (!(udev = udev_new())) {
                fprintf(stderr, "Failed to allocate udev context.\n");
                return NULL;
        }

        if (!(udev_enum =  udev_enumerate_new(udev))) {
                fprintf(stderr, "Failed to allocate enumeration object.\n");
                goto finish;
        }

        if (udev_enumerate_add_match_subsystem(udev_enum, "sound") < 0) {
                fprintf(stderr, "Failed to install subsystem match.\n");
                goto finish;
        }

        if (udev_enumerate_scan_devices(udev_enum) < 0) {
                fprintf(stderr, "Failed to enumerate devices.\n");
                goto finish;
        }

        first = udev_enumerate_get_list_entry(udev_enum);
        udev_list_entry_foreach(i, first) {
                const char *sysfs, *p;
                long l;
                char d[64];
                char *e = NULL;
                struct udev_device *dev;
                const char *ff, *class, *bus;

                sysfs = udev_list_entry_get_name(i);

                if (!(p = strrchr(sysfs, '/')))
                        continue;

                p++;

                if (strncmp(p, "card", 4) != 0)
                        continue;

                errno = 0;
                l = strtol(p + 4, &e, 10);
                if (!e || *e != 0 || errno != 0)
                        continue;

                /* Check whether this sound card has a playback device
                 * #0 (i.e. something that is not HDMI, SPDIF or
                 * something other weird.) */
                snprintf(d, sizeof(d), "/sys/class/sound/card%i/pcmC%iD0p", (int) l, (int) l);
                if (access(d, F_OK) < 0)
                        continue;

                if (!(dev = udev_device_new_from_syspath(udev, sysfs)))
                        continue;

                class = udev_device_get_property_value(dev, "SOUND_CLASS");
                ff = udev_device_get_property_value(dev, "SOUND_FORM_FACTOR");
                bus = udev_device_get_property_value(dev, "ID_BUS");

                /* Ignore modems and other non-audio sound device */
                if (class && !ca_streq(class, "sound")) {
                        udev_device_unref(dev);
                        continue;
                }

                /* Prefer "internal" devices */
                if (internal_device < 0 && ff && ca_streq(ff, "internal"))
                        internal_device = (int) l;

                /* If no "internal" device is available, prefer PCI devices */
                if (pci_device < 0 && bus && ca_streq(bus, "pci"))
                        pci_device = (int) l;

                /* If neither "internal" nor PCI devices are
                 * available, pick whatever we can find */
                if (other_device < 0)
                        other_device = (int) l;

                udev_device_unref(dev);
        }

        if (internal_device >= 0)
                asprintf(&s, "front:%i", internal_device);
        else if (pci_device >= 0)
                asprintf(&s, "front:%i", pci_device);
        else if (other_device >= 0)
                asprintf(&s, "front:%i", other_device);

finish:
        if (udev_enum)
                udev_enumerate_unref(udev_enum);

        if (udev)
                udev_unref(udev);

        return s;
}

static void finish_cb(ca_context *c, uint32_t id, int error_code, void *userdata) {
        uint64_t u = 1;

        for (;;) {
                if (write(CA_PTR_TO_INT(userdata), &u, sizeof(u)) > 0)
                        break;

                if (errno != EINTR) {
                        fprintf(stderr, "write() failed: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }
        }
}

int main(int argc, char *argv[]) {
        ca_context *c = NULL;
        ca_proplist *p = NULL;
        int ret = EXIT_FAILURE, r;
        int fd = -1;
        char *device = NULL;

        if (argc > 2) {
                fprintf(stderr, "This program expects no more than one parameter.\n");
                goto finish;
        }

        if ((fd = eventfd(0, EFD_CLOEXEC)) < 0) {
                fprintf(stderr, "Failed to create event file descriptor: %s\n", strerror(errno));
                goto finish;
        }

        if ((r = ca_context_create(&c)) < 0) {
                fprintf(stderr, "Failed to create context: %s\n", ca_strerror(r));
                goto finish;
        }

        if ((r = ca_context_set_driver(c, "alsa")) < 0) {
                fprintf(stderr, "Failed to set driver: %s\n", ca_strerror(r));
                goto finish;
        }

        if (!(device = find_device())) {
                ret = EXIT_SUCCESS;
                goto finish;
        }

        if ((r = ca_context_change_device(c, device)) < 0) {
                fprintf(stderr, "Failed to set device: %s\n", ca_strerror(r));
                goto finish;
        }

        if ((r = ca_proplist_create(&p)) < 0) {
                fprintf(stderr, "Failed to create property list: %s\n", ca_strerror(r));
                goto finish;
        }

        if ((r = ca_proplist_sets(p, CA_PROP_EVENT_ID, argc >= 2 ? argv[1] : "system-bootup")) < 0 ||
            (r = ca_proplist_sets(p, CA_PROP_CANBERRA_CACHE_CONTROL, "never")) < 0) {
                fprintf(stderr, "Failed to set event id: %s\n", strerror(r));
                goto finish;
        }

        if ((r = ca_context_play_full(c, 0, p, finish_cb, CA_INT_TO_PTR(fd))) < 0) {
                fprintf(stderr, "Failed to play event sound: %s\n", ca_strerror(r));
                goto finish;
        }

        for (;;) {
                uint64_t u;

                if (read(fd, &u, sizeof(u)) < 0) {
                        if (errno == EINTR)
                                break;

                        fprintf(stderr, "read() failed: %s\n", strerror(errno));

                } else
                        break;
        }

        ret = EXIT_SUCCESS;

finish:
        if (c)
                ca_context_destroy(c);

        if (p)
                ca_proplist_destroy(p);

        if (fd >= 0)
                close(fd);

        free(device);

        return ret;
}
