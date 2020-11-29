// Generate a index for a web comic
#include "img-panda/comics/index.h"
#include "img-panda/comics/identify/wordpress.h"
#include "img-panda/comics/scrapers/comiceasel.h"
#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>
#include <string.h>

static void imp__wc_initial_easel_page_cb (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    puts(">>> GOT LATEST COMIC PAGE");
    imp_wc_indexer_state_t* state = pool->data;
    lxb_status_t status;
    lxb_html_document_t *document;

    document = lxb_html_document_create();
    if (document == NULL)
        goto fail;

    status = lxb_html_document_parse(document, worker->last_response.base, worker->last_response.len);
    if (status != LXB_STATUS_OK)
        goto fail;

    imp_wc_comic_easel_crawl (state, document, worker->client.url);

fail:
    // TODO FAIL
    lxb_html_document_destroy(document);
}

static void imp__wc_ident_wordpress_cb (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    puts(">>> WORDPRESS IDENTIFY");
    imp_wc_indexer_state_t* state = pool->data;
    // TODO ERROR CHECK
    imp_parse_wordpress_site_info_json(&state->metadata, (imp_buf_t *)&worker->last_response);

    switch (state->metadata.plugin.data)
    {
    case META_CMS_PL_COMICPRESS:
    case META_CMS_PL_COMIC_EASEL:
        {
            imp_url_path_t path = {
                .path = "/",
                .query = "latest",
                .fragment = "",
                .userinfo = ""
            };

            imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));

            req->url = imp_url_dup(state->url);

            imp_http_request_t *http_req = imp_http_request_init(NULL, "GET");
    
            imp_http_headers_push(&http_req->headers, "Host", state->url->host);
            imp_http_headers_push(&http_req->headers, "Connection", "keep-alive");
            imp_http_headers_push(&http_req->headers, "User-Agent", "img-panda/0.1.0");

            req->request = (imp_buf_t *)imp_http_request_serialize_with_path(http_req, NULL, (imp_url_path_t *)req->url);

            req->on_response = imp__wc_initial_easel_page_cb;

            imp_http_request_free(http_req);

            imp_http_pool_request(&state->pool, req);
        }
        break;
    
    default:
        break;
    }
}

static void imp__wc_identify_cb (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    puts(">>> IDENTIFYING");
    lxb_status_t status;
    lxb_html_document_t *document;
    imp_wc_indexer_state_t* state = pool->data;

    document = lxb_html_document_create();
    if (document == NULL)
        goto fail;

    status = lxb_html_document_parse(document, worker->last_response.base, worker->last_response.len);
    if (status != LXB_STATUS_OK)
        goto fail;

    // TODO ERROR CHECK
    imp_wc_meta_process_index(&state->metadata, document, worker->client.url);

    switch (state->metadata.cms.data)
    {
    case META_CMS_WORDPRESS:
        {
            imp_url_path_t path = {
                .path = "/wp-json",
                .query = "",
                .fragment = "",
                .userinfo = ""
            };
            imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));

            req->url = imp_url_dup(state->url);

            imp_http_request_t *http_req = imp_http_request_init(NULL, "GET");
    
            imp_http_headers_push(&http_req->headers, "Host", state->url->host);
            imp_http_headers_push(&http_req->headers, "Connection", "keep-alive");
            imp_http_headers_push(&http_req->headers, "User-Agent", "img-panda/0.1.0");

            req->request = (imp_buf_t *)imp_http_request_serialize_with_path(http_req, NULL, (imp_url_path_t *)req->url);

            imp_http_request_free(http_req);

            req->on_response = imp__wc_ident_wordpress_cb;

            imp_http_pool_request(&state->pool, req);
        }
        break;
    
    default:
        {
            // TODO: SEARCH FOR ROIs
            puts("FAILED: MANUAL IDENT");
        }
        break;
    }

    puts("*** Destroying initial HTML document");

    lxb_html_document_destroy (document);

    return;
fail:
    // TODO FAIL
    lxb_html_document_destroy(document);
};

int imp_wc_indexer_init (
    uv_loop_t *loop, 
    imp_wc_indexer_state_t *state
) {
    memset(state, 0, sizeof(imp_wc_indexer_state_t));

    state->loop = loop;

    imp_http_pool_init(loop, &state->pool, 20);  

    state->pool.data = state;  

    imp_wc_meta_index_init (&state->metadata);
};

int imp_wc_indexer_run (
    imp_wc_indexer_state_t *state, 
    const char* url, const size_t url_len, 
    imp_wc_indexer_cb on_complete
) {

    imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));

    req->url = imp_parse_url(url, url_len);

    state->url = imp_url_dup(req->url);

    imp_http_request_t *http_req = imp_http_request_init(NULL, "GET");
    
    imp_http_headers_push(&http_req->headers, "Host", req->url->host);
    imp_http_headers_push(&http_req->headers, "Connection", "keep-alive");
    imp_http_headers_push(&http_req->headers, "User-Agent", "img-panda/0.1.0");

    req->request = (imp_buf_t *)imp_http_request_serialize_with_path(http_req, NULL, (imp_url_path_t *)req->url);

    req->on_response = imp__wc_identify_cb;
    // req->on_complete

    imp_http_request_free(http_req);

    imp_http_pool_request(&state->pool, req);

    return 1;
}
