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

    imp_wc_comic_easel_crawl(state, document, worker->client.url);

fail:
    // TODO FAIL
    lxb_html_document_destroy(document);
}

static void imp__wc_ident_wordpress_cb (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    puts(">>> WORDPRESS IDENTIFY");
    imp_wc_indexer_state_t* state = pool->data;
    // TODO ERROR CHECK
    imp_parse_wordpress_site_info_json(&state->metadata, (imp_buf_t *)&worker->last_response);
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
            imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));
            memset(req, 0, sizeof(imp_http_worker_request_t));

            req->url = imp_url_clone(state->url);
            free(req->url->path);
            req->url->path = strdup("/wp-json");

            imp_http_request_t *http_req = imp_http_request_init("GET");
    
            imp_http_pool_default_headers(&http_req->headers);

            req->request = http_req;

            req->on_response = imp__wc_ident_wordpress_cb;

            imp_http_pool_request(&state->pool, req);

            switch (state->metadata.plugin.data)
            {
            case META_CMS_PL_COMICPRESS:
            case META_CMS_PL_COMIC_EASEL:
                {

                    imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));
                    memset(req, 0, sizeof(imp_http_worker_request_t));

                    req->url = imp_url_clone(state->url);
                    free(req->url->path);
                    req->url->path = strdup("/");
                    free(req->url->query);
                    req->url->query = strdup("latest");

                    imp_http_request_t *http_req = imp_http_request_init("GET");

                    imp_http_pool_default_headers(&http_req->headers);

                    req->request = http_req;

                    req->on_response = imp__wc_initial_easel_page_cb;

                    imp_http_pool_request(&state->pool, req);
                }
                break;
            
            default:
                break;
            }
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

static int imp__strip_cmp(const void *a, const void *b, void *udata) {
    imp_wc_meta_strip_t *sa = *(imp_wc_meta_strip_t **)a;
    imp_wc_meta_strip_t *sb = *(imp_wc_meta_strip_t **)b;

    return strcmp(sa->src_url->fragment, sb->src_url->fragment) &&
    strcmp(sa->src_url->host, sb->src_url->host) &&
    strcmp(sa->src_url->path, sb->src_url->path) &&
    strcmp(sa->src_url->port, sb->src_url->port) &&
    strcmp(sa->src_url->query, sb->src_url->query) &&
    strcmp(sa->src_url->schema, sb->src_url->schema) &&
    strcmp(sa->src_url->userinfo, sb->src_url->userinfo);
}

static uint64_t imp__strip_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    imp_wc_meta_strip_t *strip = *(imp_wc_meta_strip_t **)item;
    const char *format = "%s%s%s%s%s%s";
    int len;
    len = snprintf(NULL, 0, format, 
        strip->src_url->host, 
        strip->src_url->port, 
        strip->src_url->query, 
        strip->src_url->schema, 
        strip->src_url->userinfo, 
        strip->src_url->path);
    char *key = malloc(len + 1);
    snprintf(key, len + 1, format, 
        strip->src_url->host,
        strip->src_url->port, 
        strip->src_url->query, 
        strip->src_url->schema, 
        strip->src_url->userinfo,
        strip->src_url->path);
    
    if (key[len - 1] == '/')
        len--;  // ignore trailing / as it results in the same path

    uint64_t hash = hashmap_sip(key, len, seed0, seed1);

    free(key);

    return hash;
}

static int imp__chapter_cmp(const void *a, const void *b, void *udata) {
    imp_wc_meta_chapter_t **ua = a;
    imp_wc_meta_chapter_t **ub = b;
    return strcmp((*ua)->id, (*ub)->id);
}

static uint64_t imp__chapter_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    imp_wc_meta_chapter_t *chapter = *(imp_wc_meta_chapter_t **)item;
    return hashmap_sip(chapter->id, chapter->id_len, seed0, seed1);
}

int imp_wc_indexer_init (
    uv_loop_t *loop, 
    imp_wc_indexer_state_t *state
) {
    memset(state, 0, sizeof(imp_wc_indexer_state_t));

    state->loop = loop;

    imp_http_pool_init(loop, &state->pool, 20);  

    state->pool.data = state;  

    state->content = hashmap_new(sizeof(imp_wc_meta_strip_t), 256, 0, 0, imp__strip_hash, imp__strip_cmp, NULL);
    state->chapters = hashmap_new(sizeof(imp_wc_meta_chapter_t), 0, 0, 0, imp__chapter_hash, imp__chapter_cmp, NULL);

    imp_wc_meta_index_init (&state->metadata);
};

static int count = 0;

static void imp__wc_comic_dl (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    imp_wc_indexer_state_t *state = worker->last_request->data;
    imp_wc_meta_strip_t *strip = worker->last_request_data;
    if (state->on_strip_image != NULL)
        state->on_strip_image(state, strip, (uv_buf_t *)&worker->last_response);
}

int imp_wc_download_image (imp_wc_indexer_state_t *state, imp_wc_meta_strip_t *strip) {
    imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));
    memset(req, 0, sizeof(imp_http_worker_request_t));

    req->url = imp_url_dup(strip->img_url);

    imp_http_request_t *http_req = imp_http_request_init("GET");
    
    imp_http_pool_default_headers(&http_req->headers);

    http_req->data = state;

    req->request = http_req;

    req->data = strip;

    req->on_response = imp__wc_comic_dl;

    imp_http_pool_request(&state->pool, req);
}

int imp_wc_indexer_run (
    imp_wc_indexer_state_t *state, 
    const char* url, const size_t url_len, 
    imp_wc_indexer_cb on_complete
) {

    imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));
    memset(req, 0, sizeof(imp_http_worker_request_t));

    req->url = imp_parse_url(url, url_len);

    state->url = imp_url_dup(req->url);

    imp_http_request_t *http_req = imp_http_request_init("GET");
    
    imp_http_pool_default_headers(&http_req->headers);

    req->request = http_req;

    req->on_response = imp__wc_identify_cb;
    // req->on_complete

    imp_http_pool_request(&state->pool, req);

    return 1;
}

bool imp__strip_free_iter (const void *item, void *udata) {
    imp_wc_meta_strip_t *strip = *(imp_wc_meta_strip_t **)item;
    imp_wc_meta_strip_free(strip);
    free(strip);

    return 1;
}

bool imp__chapter_free_iter (const void *item, void *udata) {
    imp_wc_meta_chapter_t *chapter = *(imp_wc_meta_chapter_t **)item;
    free(chapter->name);
    free(chapter->id);
    imp_url_free(chapter->url);
    free(chapter);

    return 1;
}

int imp_wc_indexer_shutdown (imp_wc_indexer_state_t *state) {
    hashmap_scan(state->content, imp__strip_free_iter, NULL);
    hashmap_scan(state->chapters, imp__chapter_free_iter, NULL);

    imp_wc_meta_index_free(&state->metadata);

    imp_url_free(state->url);

    imp_http_pool_shutdown(&state->pool);

    free(state);
};
