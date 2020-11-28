// Generate a index for a web comic
#include "img-panda/comics/index.h"
#include "img-panda/comics/identify/wordpress.h"
#include "img-panda/comics/scrapers/comiceasel.h"
#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>
#include <string.h>

static void imp__wc_client_status_cb (imp_http_client_t *client, imp_net_status_t *err) {
    if (err->type != FA_NET_E_OK) {
        char buf[256] = {0};
        ERR_error_string(ERR_get_error(), buf);
        printf("!!! Status %d %ld %s\n\n", err->type, err->code, buf);
        // TODO
    };
};

static void imp__wc_client_write_cb (imp_http_client_t *client, imp_net_status_t *err) {
    if (err->type != FA_NET_E_OK) {
        // TODO ERROR
    };
    imp_wc_indexer_state_t *state = client->data;
    imp_http_request_serialize_free(state->last_request);
};

inline
static void imp__wc_make_request (imp_wc_indexer_state_t *state, imp_url_path_t *path) {
    imp_http_request_t *req = imp_http_request_init(&state->client, "GET");
    
    imp_http_headers_push(&req->headers, "Connection", "keep-alive");
    imp_http_headers_push(&req->headers, "User-Agent", "img-panda/0.1.0");

    putc('\n', stdout);

    uv_buf_t *resbuf = imp_http_request_serialize_with_path(req, NULL, path);

    fwrite(resbuf->base, resbuf->len, sizeof(char), stdout);

    imp_http_request_free(req);
    
    state->last_request = resbuf;

    imp_http_client_write(&state->client, resbuf, imp__wc_client_write_cb);
}

static void imp__wc_client_ready_cb (imp_http_client_t *client) {
    imp_wc_indexer_state_t *state = client->data;
    puts(">>> HTTP CLIENT READY\n");

    imp_http_request_t *req = imp_http_request_init(client, "GET");

    imp_http_headers_push(&req->headers, "Connection", "keep-alive");
    imp_http_headers_push(&req->headers, "User-Agent", "img-panda/0.1.0");

    uv_buf_t *resbuf = imp_http_request_serialize(req, NULL);

    fwrite(resbuf->base, resbuf->len, sizeof(char), stdout);

    imp_http_request_free(req);
    
    state->last_request = resbuf;

    imp_http_client_write(client, resbuf, imp__wc_client_write_cb);
};

static void imp__wc_on_close_redirect (uv_handle_t* handle) {
    imp_wc_indexer_state_t *state = (imp_wc_indexer_state_t *)((imp_http_client_t *)handle->data)->data;
    printf("!!! Redirecting to: %s\n\n", state->redirect_url->host);
    state->client.url = state->redirect_url;
    if (imp_http_client_connect (&state->client, *imp__wc_client_status_cb, *imp__wc_client_ready_cb)) {
        // TODO: EXIT NO SHUTDOWN
    };
};

static void imp__wc_initial_easel_page_cb (imp_wc_indexer_state_t *state) {
    puts(">>> GOT LATEST COMIC PAGE");
    lxb_status_t status;
    lxb_html_document_t *document;

    document = lxb_html_document_create();
    if (document == NULL)
        goto fail;

    status = lxb_html_document_parse(document, state->last_response.base, state->last_response.len);
    if (status != LXB_STATUS_OK)
        goto fail;

    imp_wc_comic_easel_crawl (state, document, state->client.url);

fail:
    // TODO FAIL
    lxb_html_document_destroy(document);
}

static void imp__wc_ident_wordpress_cb (imp_wc_indexer_state_t *state) {
    puts(">>> WORDPRESS IDENTIFY");
    // TODO ERROR CHECK
    imp_parse_wordpress_site_info_json(&state->metadata, (imp_buf_t *)&state->last_response);

    switch (state->metadata.plugin.data)
    {
    case META_CMS_PL_COMICPRESS:
        {

        }
        break;

    case META_CMS_PL_COMIC_EASEL:
        {
            state->on_response = imp__wc_initial_easel_page_cb;

            imp_url_path_t path = {
                .path = "/",
                .query = "latest",
                .fragment = "",
                .userinfo = ""
            };

            imp__wc_make_request(state, &path);
        }
        break;
    
    default:
        break;
    }
}

static void imp__wc_identify_cb (imp_wc_indexer_state_t *state) {
    puts(">>> IDENTIFYING");
    lxb_status_t status;
    lxb_html_document_t *document;

    document = lxb_html_document_create();
    if (document == NULL)
        goto fail;

    status = lxb_html_document_parse(document, state->last_response.base, state->last_response.len);
    if (status != LXB_STATUS_OK)
        goto fail;

    // TODO ERROR CHECK
    imp_wc_meta_process_index (&state->metadata, document, state->client.url);

    switch (state->metadata.cms.data)
    {
    case META_CMS_WORDPRESS:
        {
            state->on_response = imp__wc_ident_wordpress_cb;

            imp_url_path_t path = {
                .path = "/wp-json",
                .query = "",
                .fragment = "",
                .userinfo = ""
            };

            imp__wc_make_request(state, &path);
        }
        break;
    
    default:
        {
            // TODO: SEARCH FOR ROIs
            puts("FAILED: MANUAL IDENT");
        }
        break;
    }

    puts("??? Destroying initial HTML document\n");

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
    imp_http_client_init(loop, &state->client);

    state->client.parser_settings.on_body = *imp__wc_on_body_recv;
    state->client.parser_settings.on_message_complete = *imp__wc_on_message_complete;
    state->client.parser_settings.on_status = *imp__wc_on_status_recv;
    state->client.parser_settings.on_header_field = *imp__wc_on_header_field;
    state->client.parser_settings.on_header_value = *imp__wc_on_header_value;

    state->client.settings.keep_alive = 1;
    state->client.data = state;

    imp_wc_meta_index_init (&state->metadata);

    // set default response buffer size to the default https buffer size (8KiB)
    state->last_response.base = calloc(sizeof(char), FA_HTTPS_BUF_LEN);
    state->last_response.len = 0;
    state->last_response.size = FA_HTTPS_BUF_LEN;

    state->on_response = imp__wc_identify_cb;
};

int imp_wc_indexer_run (
    imp_wc_indexer_state_t *state, 
    const char* url, const size_t url_len, 
    imp_wc_indexer_cb on_complete
) {
    state->client.url = imp_parse_url(url, url_len);
    if (state->client.url == NULL) return 0;

    if (imp_http_client_connect (&state->client, *imp__wc_client_status_cb, *imp__wc_client_ready_cb)) {
        return 0;
    };

    return 1;
}
