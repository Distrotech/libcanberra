#ifndef foocanberramacrohfoo
#define foocanberramacrohfoo

#include <stdio.h>
#include <assert.h>

#ifdef __GNUC__
#define CA_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define CA_PRETTY_FUNCTION ""
#endif

#define ca_return_if_fail(expr) \
    do { \
        if (!(expr)) { \
             fprintf(stderr, "%s: Assertion <%s> failed.\n", CA_PRETTY_FUNCTION, #expr ); \
            return; \
        } \
    } while(0)

#define ca_return_val_if_fail(expr, val) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "%s: Assertion <%s> failed.\n", CA_PRETTY_FUNCTION, #expr ); \
            return (val); \
        } \
    } while(0)

#define ca_assert assert

/* An assert which guarantees side effects of x */
#ifdef NDEBUG
#define ca_assert_se(x) x
#else
#define ca_assert_se(x) ca_assert(x)
#endif

#define ca_assert_not_reached() ca_assert(!"Should not be reached.")

#define ca_assert_success(x) do {               \
        int _r = (x);                           \
        ca_assert(_r == 0);                     \
    } while(0)

#define CA_ELEMENTSOF(x) (sizeof(x)/sizeof((x)[0]))

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define CA_PTR_TO_UINT(p) ((unsigned int) (unsigned long) (p))
#define CA_UINT_TO_PTR(u) ((void*) (unsigned long) (u))

#define CA_PTR_TO_UINT32(p) ((uint32_t) CA_PTR_TO_UINT(p))
#define CA_UINT32_TO_PTR(u) CA_UINT_TO_PTR((uint32_t) u)

#define CA_PTR_TO_INT(p) ((int) CA_PTR_TO_UINT(p))
#define CA_INT_TO_PTR(u) CA_UINT_TO_PTR((int) u)

#define CA_PTR_TO_INT32(p) ((int32_t) CA_PTR_TO_UINT(p))
#define CA_INT32_TO_PTR(u) CA_UINT_TO_PTR((int32_t) u)

static inline size_t ca_align(size_t l) {
    return (((l + sizeof(void*) - 1) / sizeof(void*)) * sizeof(void*));
}

#define CA_ALIGN(x) (ca_align(x))

typedef void (*ca_free_cb_t)(void *);

typedef int ca_bool_t;

#endif
