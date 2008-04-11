#ifndef foocanberrahfoo
#define foocanberrahfoo

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

/* Context object */
typedef struct ca_context ca_context_t;

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
#define CA_PROP_EVENT_X11_DISPLAY           "event.x11.display"
#define CA_PROP_EVENT_X11_XID               "event.x11.xid"
#define CA_PROP_EVENT_MOUSE_X               "event.mouse.x"
#define CA_PROP_EVENT_MOUSE_Y               "event.mouse.y"
#define CA_PROP_EVENT_MOUSE_BUTTON          "event.mouse.button"
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
#define CA_PROP_CONTROL_CACHE               "control.cache"    /* permanent, volatile, never */
#define CA_PROP_CONTROL_VOLUME              "control.volume"   /* decibel */

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
    _CA_ERROR_MAX = -7
};

/** Create an (unconnected) context object */
int ca_context_create(ca_context_t **c);

/** Connect the context. This call is implicitly called if necessary. It
 * is recommended to initialize the application.* properties before
 * issuing this call */
int ca_context_open(ca_context_t *c);

/** Destroy a (connected or unconnected) cntext object. */
int ca_context_destroy(ca_context_t *c);

/** Write one or more string properties to the context
 * object. Requires final NULL sentinel. Properties set like this will
 * be attached to both the client object of the sound server and to
 * all event sounds played or cached. */
int ca_context_change_props(ca_context_t *c, ...) CA_GCC_SENTINEL;

/** Write an arbitrary data property to the context object. */
int ca_context_change_prop(ca_context_t *c, const char *key, const void *data, size_t nbytes);

/** Remove a property from the context object again. */
int ca_context_remove_prop(ca_context_t *c, ...) CA_GCC_SENTINEL;

/** Play one event sound. id can be any numeric value which later can
 * be used to cancel an event sound that is currently being
 * played. You may use the same id twice or more times if you want to
 * cancel multiple event sounds with a single ca_context_cancel() call
 * at once. If the requested sound is not cached in the server yet
 * this call might result in the sample being uploaded temporarily or
 * permanently. */
int ca_context_play(ca_context_t *c, uint32_t id, ...) CA_GCC_SENTINEL;

/** Play one event sound, and call the specified callback function
    when completed. The callback will be called from a background
    thread. Other arguments identical to ca_context_play(). */
int ca_context_play_with_callback(ca_context_t *c, uint32_t id, ca_finish_callback_t cb, void *userdata, ...) CA_GCC_SENTINEL;

/** Cancel one or more event sounds that have been started via
 * ca_context_play(). */
int ca_context_cancel(ca_context_t *c, uint32_t id);

/** Upload the specified sample into the server and attach the
 * specified properties to it */
int ca_context_cache(ca_context_t *c, ...) CA_GCC_SENTINEL;

/** Return a human readable error string */
const char *ca_strerror(int code);

#endif
