// TODO NULL CHECKS !!
#include "img-panda/http/pool.h"
#include <string.h>

static void imp__http_pool_write_cb (imp_http_client_t *client, imp_net_status_t *err) {
    printf(">>> WORKER %p SENT REQUEST SENT\n", client->data);
    if (err->type != FA_NET_E_OK) {
        // TODO ERROR
    };
    imp_http_worker_t *state = client->data;
    imp_http_request_serialize_free(state->last_request);
};

static int imp__wc_on_status_recv (llhttp_t* parser, const char *at, size_t length) {
    imp_http_worker_t *state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    state->should_redirect = ((parser->status_code > 299) && (parser->status_code < 400));
    if (!state->should_redirect) {
        // TODO: EXIT AND SHUTDOWN if not 200
    };

    return 0;
}

static void imp__http_pool_ready_cb (imp_http_client_t *client) {
    imp_http_worker_t *state = client->data;
    printf(">>> WORKER %p CONNECTED IN POOL\n", state);
    state->is_connected = 1;
    imp_http_client_write(client, state->last_request, imp__http_pool_write_cb);
}

static void imp__http_pool_status_cb (imp_http_client_t *client, imp_net_status_t *err) {
    if (err->type != FA_NET_E_OK) {
        char buf[256] = {0};
        ERR_error_string(ERR_get_error(), buf);
        printf("!!! Status %d %ld %s\n\n", err->type, err->code, buf);
        // TODO
    };
};

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

static void imp__pool_client_status_cb (imp_http_client_t *client, imp_net_status_t *err) {
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
    if (imp_http_client_connect (&state->client, *imp__pool_client_status_cb, imp__http_pool_ready_cb)) {
        // TODO: EXIT NO SHUTDOWN
    };
};

static void imp__pool_on_close_new_host (uv_handle_t* handle) {
    imp_http_worker_t *state = (imp_http_worker_t *)((imp_http_client_t *)handle->data)->data;
    if (imp_http_client_connect (&state->client, *imp__pool_client_status_cb, imp__http_pool_ready_cb)) {
        // TODO: EXIT NO SHUTDOWN
    };
};

inline 
static imp_http_worker_t *imp__http_pool_set_idle (imp_http_pool_t *pool, imp_http_worker_t **work) {
    for (size_t x = 0; x < pool->pool_size; x++) {
        if (pool->idle_workers.workers[x] == NULL) {
            printf("!!! WORKER %p MOVED TO IDLE POOL\n", *work);
            pool->idle_workers.workers[x] = *work;
            *work = NULL;
            pool->working_workers.len--;
            pool->idle_workers.len++;
            return pool->idle_workers.workers[x];
        };
    };
    return NULL;
}


int imp__pool_on_message_complete (llhttp_t* parser) {
    imp_http_worker_t * state = (imp_http_worker_t *)((imp_http_client_t *)(parser->data))->data;
    
    // Handle redirects
    if (state->should_redirect) {
        if (state->redirect_new_host) 
            imp_http_client_shutdown (&state->client, imp__wc_on_close_redirect);
        else {
            printf("!!! Redirecting to: %s\n!!! Using existing connection!\n\n", state->redirect_url->host);
            imp_url_free(state->client.url);
            state->client.url = state->redirect_url;
            imp__http_pool_ready_cb(parser->data);
        }
        state->redirect_new_host = 0;
        state->should_redirect = 0;
        goto cleanup;
    };

    // Process the response
    if (state->on_response != NULL) {
        state->on_response(state, state->pool);
    };

    // Check if we can consume one of the queue items
    if (state->pool->queue_len > 0) {
        puts(">>> CONSUMING QUEUED REQUEST");
        imp_http_worker_request_t *req = state->pool->queue[state->pool->queue_len - 1];

        state->last_request = (uv_buf_t *)req->request;
        state->on_complete = req->on_complete;
        state->on_response = req->on_response;

        if (!strcmp(req->url->host, state->client.url->host)) {
            // has same host
            state->client.url = req->url;
            imp_http_client_connect(&state->client, imp__http_pool_status_cb, imp__http_pool_ready_cb);
        } else {
            // not same host close and reconnect
            state->client.url = req->url;
            imp_http_client_shutdown (&state->client, imp__pool_on_close_new_host);
        }

        free(req);

        state->pool->queue[state->pool->queue_len - 1] = NULL;
    } else {
        // move to the idle pool
        state = imp__http_pool_set_idle(state->pool, &state);
    };
    
cleanup:
    // reset the buffer
    state->last_response.len = 0;

    return 0;
};

int imp_http_worker_init (imp_http_pool_t *pool, imp_http_worker_t *worker) {
    memset(worker, 0, sizeof(imp_http_worker_t));
    imp_http_client_init(pool->loop, &worker->client);

    worker->client.parser_settings.on_body = *imp__wc_on_body_recv;
    worker->client.parser_settings.on_message_complete = *imp__pool_on_message_complete;
    worker->client.parser_settings.on_status = *imp__wc_on_status_recv;
    worker->client.parser_settings.on_header_field = *imp__wc_on_header_field;
    worker->client.parser_settings.on_header_value = *imp__wc_on_header_value;

    worker->client.settings.keep_alive = 1;
    worker->client.data = worker;

    worker->pool = pool;

    // set default response buffer size to the default https buffer size (8KiB)
    worker->last_response.base = calloc(sizeof(char), FA_HTTPS_BUF_LEN);
    worker->last_response.len = 0;
    worker->last_response.size = FA_HTTPS_BUF_LEN;

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

int imp_http_pool_init (uv_loop_t *loop, imp_http_pool_t *pool, size_t worker_count) {
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
};

inline 
static int imp__http_pool_set_working (imp_http_pool_t *pool, imp_http_worker_t **idle) {
    for (size_t x = 0; x < pool->pool_size; x++) {
        if (pool->working_workers.workers[x] == NULL) {
            printf("!!! WORKER %p MOVED TO WORK POOL\n", *idle);
            pool->working_workers.workers[x] = *idle;
            *idle = NULL;
            pool->idle_workers.len--;
            pool->working_workers.len++;
            return 1;
        };
    };
    return 0;
}

int imp_http_pool_request (imp_http_pool_t *pool, imp_http_worker_request_t *request) {
    if (pool->idle_workers.len > 0) {
        for (size_t i = 0; i < pool->idle_workers.len; i++) {
            if ((pool->idle_workers.workers[i] != NULL) &&
                pool->idle_workers.workers[i]->is_connected && 
                !strcmp(request->url->host, pool->idle_workers.workers[i]->client.url->host)) 
            {
                // request can be submitted immediately
                pool->idle_workers.workers[i]->last_request = (uv_buf_t *)request->request;
                pool->idle_workers.workers[i]->on_complete = request->on_complete;
                pool->idle_workers.workers[i]->on_response = request->on_response;
                imp_url_free(request->url);
                imp_http_client_write(&pool->idle_workers.workers[i]->client, (uv_buf_t *)request->request, imp__http_pool_write_cb);

                free(request);

                return imp__http_pool_set_working(pool, &pool->idle_workers.workers[i]);
            };
        };

        for (size_t i = 0; i < pool->idle_workers.len; i++) {
            if ((pool->idle_workers.workers[i] != NULL) && 
                (pool->idle_workers.workers[i]->is_connected == 0)) 
            {
                // is not connected
                puts("NOT CONNECTED");
                pool->idle_workers.workers[i]->last_request = (uv_buf_t *)request->request;
                pool->idle_workers.workers[i]->on_complete = request->on_complete;
                pool->idle_workers.workers[i]->on_response = request->on_response;

                if (pool->idle_workers.workers[i]->client.url != NULL)
                    imp_url_free(pool->idle_workers.workers[i]->client.url);

                pool->idle_workers.workers[i]->client.url = request->url;
                imp_http_client_connect(&pool->idle_workers.workers[i]->client, imp__http_pool_status_cb, imp__http_pool_ready_cb);

                free(request);
                
                return imp__http_pool_set_working(pool, &pool->idle_workers.workers[i]);
            };
        };

        for (size_t i = 0; i < pool->idle_workers.len; i++) {
            if (pool->idle_workers.workers[i] != NULL) 
            {
                pool->idle_workers.workers[i]->last_request = (uv_buf_t *)request->request;
                pool->idle_workers.workers[i]->on_complete = request->on_complete;
                pool->idle_workers.workers[i]->on_response = request->on_response;

                if (pool->idle_workers.workers[i]->client.url != NULL)
                    imp_url_free(pool->idle_workers.workers[i]->client.url);

                pool->idle_workers.workers[i]->client.url = request->url;
                imp_http_client_shutdown (&pool->idle_workers.workers[i]->client, imp__pool_on_close_new_host);

                free(request);

                return imp__http_pool_set_working(pool, &pool->idle_workers.workers[i]);
            }
        }
        return 0;
    } else {
        if (pool->queue_len >= pool->queue_size)
            imp__http_pool_queue_grow(pool);
        pool->queue[pool->queue_len] = request;
        pool->queue_len++;
        return 1;
    };
};

void imp_http_pool_shutdown (imp_http_pool_t *pool);
