#ifndef foocanberracommonh
#define foocanberracommonh

#include "canberra.h"

#define N_HASHTABLE 39

typedef struct ca_prop {
    char *key;
    size_t nbytes;
    struct ca_prop *next_in_slot, *next_item, *prev_item;
} ca_prop;

struct ca_context {
    ca_bool_t opened;
    ca_prop *prop_hashtable[N_HASHTABLE];
    ca_prop *first_item;
    void *private;
};

#define CA_PROP_DATA(p) ((void*) ((char*) (p) + CA_ALIGN(sizeof(ca_prop))))

ca_prop* ca_context_get(ca_context *c, const char *key);
const char* ca_context_gets(ca_context *c, const char *key);


#endif
