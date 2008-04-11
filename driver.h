#ifndef foocanberradriverhfoo
#define foocanberradriverhfoo

#include "canberra.h"

int driver_open(ca_context *c);
int driver_destroy(ca_context *c);

int driver_set(ca_context *c, const char *key, const void* data, size_t nbytes);
int driver_unset(ca_context *c, const char *key);

int driver_play(ca_context *c, uint32_t id, ca_notify_cb_t cb, void *userdata, va_list ap);
int driver_cancel(ca_context *c, uint32_t id);
int driver_cache(ca_context *c, va_list ap);

#endif
