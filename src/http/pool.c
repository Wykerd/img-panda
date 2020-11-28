#include "img-panda/http/pool.h"
#include <string.h>

static int imp__wc_on_status_recv (llhttp_t* parser, const char *at, size_t length) {
    imp_http_worker_t *state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    state->should_redirect = ((parser->status_code > 299) && (parser->status_code < 400));
    if (!state->should_redirect) {
        // TODO: EXIT AND SHUTDOWN if not 200
    };

    return 0;
}

int imp__wc_on_body_recv (llhttp_t* parser, const char *at, size_t length) {
    imp_http_worker_t * state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    
    size_t new_size = state->last_response.len + length;

    if (new_size > state->last_response.size) {
        state->last_response.size = new_size;
        state->last_response.base = realloc(state->last_response.base, new_size);
    };

    memcpy(state->last_response.base + state->last_response.len, at, length);
    state->last_response.len += length;

    return 0;
};

int imp__wc_on_header_field (llhttp_t* parser, const char *at, size_t length) {
    imp_http_worker_t * state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    if (state->should_redirect && (length == 8))
        state->is_location_header = imp_http_header_is_equal(at, length, "location", 8);
    else
        state->is_location_header = 0;

    return 0;
}

int imp__wc_on_header_value (llhttp_t* parser, const char *at, size_t length) {
    imp_http_worker_t * state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    if (state->is_location_header) {
        state->redirect_url = imp_parse_url(at, length);
        if (state->redirect_url == NULL) {
            // TODO: EXIT AND SHUTDOWN
        } else 
            state->redirect_new_host = imp_url_same_host(state->redirect_url, state->client.url);
    }

    return 0;
}

static void imp__wc_client_status_cb (imp_http_client_t *client, imp_net_status_t *err) {
    if (err->type != FA_NET_E_OK) {
        char buf[256] = {0};
        ERR_error_string(ERR_get_error(), buf);
        printf("!!! Status %d %ld %s\n\n", err->type, err->code, buf);
        // TODO
    };
};

static void imp__wc_on_close_redirect (uv_handle_t* handle) {
    imp_http_worker_t *state = (imp_http_worker_t *)((imp_http_client_t *)handle->data)->data;
    printf("!!! Redirecting to: %s\n\n", state->redirect_url->host);
    state->client.url = state->redirect_url;
    if (imp_http_client_connect (&state->client, *imp__wc_client_status_cb, *imp__wc_client_ready_cb)) {
        // TODO: EXIT NO SHUTDOWN
    };
};

int imp__wc_on_message_complete (llhttp_t* parser) {
    imp_http_worker_t * state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    
    if (state->should_redirect) {
        if (state->redirect_new_host) 
            imp_http_client_shutdown (&state->client, imp__wc_on_close_redirect);
        else {
            printf("!!! Redirecting to: %s\n!!! Using existing connection!\n\n", state->redirect_url->host);
            imp_url_free(state->client.url);
            state->client.url = state->redirect_url;
            imp__wc_client_ready_cb(parser->data);
        }
        state->redirect_new_host = 0;
        state->should_redirect = 0;
        goto cleanup;
    };

    // Process the response
    if (state->on_response != NULL) {
        state->on_response(state, state->pool);
    };

cleanup:
    // reset the buffer
    memset(state->last_response.base, 0, state->last_response.size);
    state->last_response.len = 0;

    return 0;
};

int imp_http_worker_init (imp_http_pool_t *pool, imp_http_worker_t *worker) {
    memset(worker, 0, sizeof(imp_http_worker_t));
    imp_http_client_init(pool->loop, &worker->client);

    worker->client.parser_settings.on_body = *imp__wc_on_body_recv;
    worker->client.parser_settings.on_message_complete = *imp__wc_on_message_complete;
    worker->client.parser_settings.on_status = *imp__wc_on_status_recv;
    worker->client.parser_settings.on_header_field = *imp__wc_on_header_field;
    worker->client.parser_settings.on_header_value = *imp__wc_on_header_value;

    worker->client.settings.keep_alive = 1;
    worker->client.data = worker;

    worker->pool = pool;

    return 1;
}

static int imp__http_pool_queue_grow (imp_http_pool_t *pool) {
    size_t old_size = pool->queue_size;
    pool->queue_size *= 1.5; // growth factor
    pool->queue = realloc(pool->queue, sizeof(imp_http_worker_request_t *) * pool->queue_size);

    if (pool->queue == NULL)
        return 0;
    
    return 1;
}

int imp_http_pool_init (uv_loop_t *loop, imp_http_pool_t *pool, size_t worker_count, imp_url_t *url) {
    memset(pool, 0, sizeof(imp_http_pool_t));

    pool->loop = loop;
    pool->pool_size = worker_count;

    pool->queue_size = worker_count;
    pool->queue = malloc(sizeof(imp_http_worker_request_t *) * pool->queue_size);
    if (pool->queue == NULL)
        return 0;
    
    pool->queue_len = 0;

    pool->idle_workers.len = worker_count;
    pool->working_workers.len = 0;

    pool->idle_workers.workers = malloc(sizeof(imp_http_worker_t *) * worker_count);

    pool->working_workers.workers = malloc(sizeof(imp_http_worker_t *) * worker_count);

    for (size_t i = 0; i < worker_count; i++) {
        pool->idle_workers.workers[i] = malloc(sizeof(imp_http_worker_t));
        if (pool->idle_workers.workers[i] == NULL)
            return 0;
        imp_http_worker_init(pool, pool->idle_workers.workers[i]);
    };

    return 1;
}

int imp_http_pool_request (imp_http_pool_t *pool, imp_http_worker_request_t *request) {
    if (pool->idle_workers.len > 0) {
        for (size_t i = 0; i < pool->idle_workers.len; i++) {
            if (pool->idle_workers.workers[i]->is_connected && !strcmp(request->url->host, pool->idle_workers.workers[i]->client.url->host)) {
                
            }
        };
    }
}

void imp_http_pool_shutdown (imp_http_pool_t *pool);
