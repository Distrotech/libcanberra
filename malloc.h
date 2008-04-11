#ifndef foosydneymallochfoo
#define foocanberramallochfoo

#include <stdlib.h>
#include <string.h>

#define ca_malloc malloc
#define ca_free free
#define ca_malloc0(size) calloc(1, (size))
#define ca_strdup strdup
#define ca_strndup strndup

void* ca_memdup(const void* p, size_t size);

#define ca_new(t, n) ((t*) ca_malloc(sizeof(t)*(n)))
#define ca_new0(t, n) ((t*) ca_malloc0(sizeof(t)*(n)))
#define ca_newdup(t, p, n) ((t*) ca_memdup(p, sizeof(t)*(n)))

#endif
~
