#ifndef IMP_HTTP_POOL_H
#define IMP_HTTP_POOL_H

#include "img-panda/common.h"
#include "http.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct imp_http_worker_s imp_http_worker_t;
typedef struct imp_http_pool_s imp_http_pool_t;

typedef void (*imp_http_pool_cb)(imp_http_worker_t *worker, imp_http_pool_t *pool, int status, int errcode);

typedef void (*imp_http_pool_status_cb)(imp_http_worker_t *worker, imp_http_pool_t *pool);

struct imp_http_worker_s {
    imp_http_client_t client;

    // Events
    imp_http_pool_cb _on_complete; // Internal use only
    imp_http_pool_cb on_complete;
    imp_http_pool_status_cb _on_response; // Internal use only
    imp_http_pool_status_cb on_response;

    // Redirect
    int should_redirect;
    int is_location_header;
    int redirect_new_host;
    imp_url_t *redirect_url;

    // HTTP
    uv_buf_t *last_request;
    imp_reusable_buf_t last_response;

    // Status
    int working;
    int is_connected;
    time_t created_at;

    // Parent
    imp_http_pool_t *pool;
};

typedef struct imp_http_worker_list_s {
    imp_http_worker_t **workers;
    size_t len;
} imp_http_worker_list_t;

typedef struct imp_http_worker_request_s {
    imp_buf_t *request; // buffer content and buffer itself is freed internally!
    imp_url_t *url; // url is to be duped as it is freed internally!
    imp_http_pool_cb on_complete;
    imp_http_pool_status_cb on_response;
    void* data; // opaque
} imp_http_worker_request_t;

typedef struct imp_http_pool_s {
    uv_loop_t *loop;

    imp_http_worker_list_t working_workers;
    imp_http_worker_list_t idle_workers;
    size_t pool_size;

    imp_http_worker_request_t **queue;
    size_t queue_len;
    size_t queue_size;
} imp_http_pool_t;

int imp_http_worker_init (imp_http_pool_t *pool, imp_http_worker_t *worker);
int imp_http_pool_init (uv_loop_t *loop, imp_http_pool_t *pool, size_t worker_count, imp_url_t *url);

int imp_http_pool_request (imp_http_pool_t *pool, imp_http_worker_request_t *request);

void imp_http_pool_shutdown (imp_http_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif
