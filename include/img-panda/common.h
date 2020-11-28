#ifndef IMP_UTILS_H
#define IMP_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "hashmap.h"

typedef struct hashmap imp_hashmap_t;

#define CHK_EQ(x, y, z, m) \
    x = y; \
    if (x != z) \
        return m; 

#define CHK_EQ_CB(x, y, z, m, cb) \
    x = y; \
    if (x != z) { \
        cb; \
        return m; \
    }

#define CHK_N_EQ(x, y, z, m) \
    x = y; \
    if (x == z) \
        return m; 


#define CHK_N_EQ_CB(x, y, z, m, cb) \
    x = y; \
    if (x == z) { \
        cb; \
        return m; \
    }

#define DUMP_ATTRIBUTE(x, y) \
    fprintf(stdout, "??? Dump attribute " y ": "); \
    fwrite(x->value->data, x->value->length, sizeof(lxb_char_t), stdout); \
    putc('\n', stdout)

typedef struct imp_buf_s {
    char* base;
    size_t len;
} imp_buf_t;

typedef struct imp_reusable_buf_s {
    char* base;
    size_t len;
    size_t size;
} imp_reusable_buf_t;

// Function provide from https://stackoverflow.com/questions/8584644/strstr-for-a-string-that-is-not-null-terminated
/* binary search in memory */
int memsearch(const char *hay, int haysize, const char *needle, int needlesize);

#ifdef __cplusplus
}
#endif
#endif