#ifndef foocanberrahfoo
#define foocanberrahfoo

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

#include <sys/types.h>
#include <sys/param.h>
#include <inttypes.h>

/*

  Requirements & General observations:

    - Property set extensible. To be kept in sync with PulseAudio and libsydney.
    - Property keys need to be valid UTF-8, text values, too.
    - Will warn if application.name or application.id not set.
    - Will fail if event.id not set
    - Fully thread safe, not async-signal safe
    - Error codes are returned immediately, as negative integers
    - If the control.cache property is set it will control whether the
      specific sample will be cached in the server:

       * permanent: install the sample permanently in the server (for usage in gnome-session)
       * volatile:  install the sample temporarily in the server (will be expelled from cache on cache pressure or after timeout)
       * never:     never cache the sample in the server, always stream

      control.cache will default to "volatile" for ca_context_cache() and "never" for ca_context_play().
      control.cache is only a hint, the server may ignore this value
    - application.process.* will be filled in automatically but may be overwritten by the client.
      They thus should not be used for authentication purposes.
    - The property list attached to the client object in the sound
      server will be those specified via ca_context_prop_xx().
    - The property list attached to cached samples in the sound server
      will be those specified via ca_context_prop_xx() at sample upload time,
      combined with those specified directly at the _cache() function call
      (the latter potentially overwriting the former).
    - The property list attached to sample streams in the sound server
      will be those attached to the cached sample (only if the event
      sound is cached, of course) combined (i.e. potentially
      overwritten by) those set via ca_context_prop_xx() at play time,
      combined (i.e. potentially overwritten by) those specified
      directly at the _play() function call.
    - It is recommended to set application.* once before calling
      _open(), and media.* event.* at both cache and play time.

*/

#ifdef __GNUC__
#define CA_GCC_PRINTF_ATTR(a,b) __attribute__ ((format (printf, a, b)))
#else
/** If we're in GNU C, use some magic for detecting invalid format strings */
#define CA_GCC_PRINTF_ATTR(a,b)
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define CA_GCC_SENTINEL __attribute__ ((sentinel))
#else
/** Macro for usage of GCC's sentinel compilation warnings */
#define CA_GCC_SENTINEL
#endif

/** Context, event, and playback properties */
#define CA_PROP_MEDIA_NAME                  "media.name"
#define CA_PROP_MEDIA_TITLE                 "media.title"
#define CA_PROP_MEDIA_ARTIST                "media.artist"
#define CA_PROP_MEDIA_LANGUAGE              "media.language"
#define CA_PROP_MEDIA_FILENAME              "media.filename"
#define CA_PROP_MEDIA_ICON                  "media.icon"
#define CA_PROP_MEDIA_ICON_NAME             "media.icon_name"
#define CA_PROP_MEDIA_ROLE                  "media.role"
#define CA_PROP_EVENT_ID                    "event.id"
#define CA_PROP_EVENT_MOUSE_X               "event.mouse.x"
#define CA_PROP_EVENT_MOUSE_Y               "event.mouse.y"
#define CA_PROP_EVENT_MOUSE_BUTTON          "event.mouse.button"
#define CA_PROP_WINDOW_X11_DISPLAY          "window.x11.display"
#define CA_PROP_WINDOW_X11_XID              "window.x11.xid"
#define CA_PROP_APPLICATION_NAME            "application.name"
#define CA_PROP_APPLICATION_ID              "application.id"
#define CA_PROP_APPLICATION_VERSION         "application.version"
#define CA_PROP_APPLICATION_ICON            "application.icon"
#define CA_PROP_APPLICATION_ICON_NAME       "application.icon_name"
#define CA_PROP_APPLICATION_LANGUAGE        "application.language"
#define CA_PROP_APPLICATION_PROCESS_ID      "application.process.id"
#define CA_PROP_APPLICATION_PROCESS_BINARY  "application.process.binary"
#define CA_PROP_APPLICATION_PROCESS_USER    "application.process.user"
#define CA_PROP_APPLICATION_PROCESS_HOST    "application.process.host"
#define CA_PROP_CANBERRA_CONTROL_CACHE      "canberra.control.cache"    /* permanent, volatile, never */
#define CA_PROP_CANBERRA_CONTROL_VOLUME     "canberra.control.volume"   /* decibel */

/* Context object */
typedef struct ca_context ca_context;

/** Playback completion event callback */
typedef void ca_finish_callback_t(ca_context *c, uint32_t id, void *userdata);

/** Error codes */
enum {
    CA_SUCCESS = 0,
    CA_ERROR_NOT_SUPPORTED = -1,
    CA_ERROR_INVALID = -2,
    CA_ERROR_STATE = -3,
    CA_ERROR_OOM = -4,
    CA_ERROR_NO_DRIVER = -5,
    CA_ERROR_SYSTEM = -6,
    CA_ERROR_CORRUPT = -7,
    CA_ERROR_TOOBIG = -8,
    _CA_ERROR_MAX = -9
};

typedef struct ca_proplist ca_proplist;

int ca_proplist_create(ca_proplist **c);
int ca_proplist_destroy(ca_proplist *c);
int ca_proplist_sets(ca_proplist *p, const char *key, const char *value);
int ca_proplist_setf(ca_proplist *p, const char *key, const char *format, ...) CA_GCC_PRINTF_ATTR(3,4);
int ca_proplist_set(ca_proplist *p, const char *key, const void *data, size_t nbytes);

/** Create an (unconnected) context object */
int ca_context_create(ca_context **c);

/** Connect the context. This call is implicitly called if necessary. It
 * is recommended to initialize the application.* properties before
 * issuing this call */
int ca_context_open(ca_context *c);

/** Destroy a (connected or unconnected) cntext object. */
int ca_context_destroy(ca_context *c);

/** Write one or more string properties to the context
 * object. Requires final NULL sentinel. Properties set like this will
 * be attached to both the client object of the sound server and to
 * all event sounds played or cached. */
int ca_context_change_props(ca_context *c, ...) CA_GCC_SENTINEL;

/** Write an arbitrary data property to the context object. */
int ca_context_change_props_full(ca_context *c, ca_proplist *p);

/** Play one event sound. id can be any numeric value which later can
 * be used to cancel an event sound that is currently being
 * played. You may use the same id twice or more times if you want to
 * cancel multiple event sounds with a single ca_context_cancel() call
 * at once. It is recommended to pass 0 for the id if the event sound
 * shall never be canceled. If the requested sound is not cached in
 * the server yet this call might result in the sample being uploaded
 * temporarily or permanently. */
int ca_context_play(ca_context *c, uint32_t id, ...) CA_GCC_SENTINEL;

/** Play one event sound, and call the specified callback function
    when completed. The callback will be called from a background
    thread. Other arguments identical to ca_context_play(). */
int ca_context_play_full(ca_context *c, uint32_t id, ca_proplist *p, ca_finish_callback_t cb, void *userdata);

/** Upload the specified sample into the server and attach the
 * specified properties to it */
int ca_context_cache(ca_context *c, ...) CA_GCC_SENTINEL;

/** Upload the specified sample into the server and attach the
 * specified properties to it */
int ca_context_cache_full(ca_context *c, ca_proplist *p);

/** Cancel one or more event sounds that have been started via
 * ca_context_play(). */
int ca_context_cancel(ca_context *c, uint32_t id);

/** Return a human readable error string */
const char *ca_strerror(int code);

#endif
