// Adapted from FireAnt source
#ifndef IMP_URL_H
#define IMP_URL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define FA_PATH_FIELDS \
    char* path; \
    char* query; \
    char* fragment; \
    char* userinfo; \

typedef struct imp_url_path_s {
    FA_PATH_FIELDS
} imp_url_path_t;

typedef struct imp_url_s {
    FA_PATH_FIELDS
    char* schema;
    char* host;
    char* port;
    size_t ref;
} imp_url_t;

#define imp_url_same_host(x, y) (!!strcmp((x)->host, (y)->host) && !!strcmp((x)->port, (y)->port))

imp_url_t *imp_parse_url (const char* buf, size_t buflen);
imp_url_t *imp_url_dup(imp_url_t *url);
imp_url_t *imp_url_clone (imp_url_t *url);
void imp_url_free (imp_url_t *url);

#ifdef __cplusplus
}
#endif
#endif