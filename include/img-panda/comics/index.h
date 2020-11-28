#ifndef IMP_WC_INDEXER_H
#define IMP_WC_INDEXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "img-panda/common.h"
#include "img-panda/http/http.h"
#include "img-panda/comics/identify/metadata.h"
#include "err.h"
#include <uv.h>

typedef struct imp_wc_indexer_state_s imp_wc_indexer_state_t;

typedef void (*imp_wc_indexer_cb)(imp_wc_indexer_state_t *state, int status, int errcode);

typedef void (*imp_wc_indexer_status_cb)(imp_wc_indexer_state_t *state);

struct imp_wc_indexer_state_s {
    uv_loop_t *loop;
    imp_http_client_t client;
    
    // Events
    imp_wc_indexer_cb on_complete;
    imp_wc_indexer_status_cb on_response;
    
    // Redirect
    int should_redirect;
    int is_location_header;
    int redirect_new_host;
    imp_url_t *redirect_url;

    // HTTP
    uv_buf_t *last_request;
    imp_reusable_buf_t last_response;

    // Work pool

    // State
    imp_wc_meta_index_t metadata;
    // Download track state
    imp_hashmap_t *content;
};

typedef imp_wc_err_t (*imp_scraper_func)(imp_wc_indexer_state_t *state, lxb_html_document_t *document, imp_url_t* url);

int imp_wc_download_image (imp_wc_indexer_state_t *state, imp_wc_meta_strip_t *strip);
int imp_wc_get_chapter (imp_wc_indexer_state_t *state, imp_scraper_func scraper);

int imp_wc_indexer_init (uv_loop_t *loop, imp_wc_indexer_state_t *state);
int imp_wc_indexer_run (imp_wc_indexer_state_t *state, const char* url, const size_t url_len, imp_wc_indexer_cb on_complete);

#ifdef __cplusplus
}
#endif

#endif