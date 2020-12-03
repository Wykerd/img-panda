#include "img-panda/comics/scrapers/comiceasel.h"
#include "lexbor/html/serialize.h"
// TODO null checks

imp_wc_err_t imp_wc_comic_easel_chapter_page (imp_wc_indexer_state_t *state, lxb_html_document_t *document, 
                                              imp_url_t* url, imp_wc_meta_chapter_t *chapter) 
{
    puts("------- PROCESSING PAGE");
}

static void imp__chapter_page_dl_cb (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    imp_wc_indexer_state_t *state = worker->last_request->data;
    imp_wc_meta_chapter_t *chap = worker->last_request_data;
    lxb_status_t status;
    lxb_html_document_t *document;

    document = lxb_html_document_create();
    if (document == NULL)
        goto fail;

    status = lxb_html_document_parse(document, worker->last_response.base, worker->last_response.len);
    if (status != LXB_STATUS_OK)
        goto fail;

    imp_wc_comic_easel_chapter_page (state, document, worker->client.url, chap);

fail:
    // TODO FAIL
    lxb_html_document_destroy(document);
}

inline
static void imp__push_chapter_page (imp_wc_indexer_state_t *state, imp_wc_meta_chapter_t *chapter, const size_t page) {
    imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));
    memset(req, 0, sizeof(imp_http_worker_request_t));

    req->url = imp_url_clone(chapter->url);

    size_t path_len = strlen(req->url->path);

    req->url->path = realloc(req->url->path, path_len + 15);

    if (req->url->path[path_len - 1] != '/')
        req->url->path[path_len++] = '/';

    snprintf(req->url->path + path_len, path_len + 14, "page/%d", page);

    printf("--- DOWNLOADING PAGE %s\n", req->url->path);

    imp_http_request_t *http_req = imp_http_request_init("GET");
    
    imp_http_pool_default_headers(&http_req->headers);

    http_req->data = state;

    req->request = http_req;

    req->data = chapter;

    req->on_response = imp__chapter_page_dl_cb;

    imp_http_pool_request(&state->pool, req);
}

imp_wc_err_t imp_wc_comic_easel_chapter (imp_wc_indexer_state_t *state, lxb_html_document_t *document, 
                                         imp_url_t* url, imp_wc_meta_chapter_t *chapter)
{
    lxb_status_t status;
    imp_wc_err_t err = E_WC_OK;

    lxb_dom_collection_t *collection;

    CHK_N_EQ(collection, lxb_dom_collection_make(&document->dom_document, 128), NULL, E_WC_CREATE_COLLECTION);

    size_t path_len = strlen(url->path);
    size_t chapter_pre_len = path_len + 7;

    char *chapter_pre = calloc(sizeof(char), chapter_pre_len);

    if (url->path[path_len - 1] == '/') {
        path_len--;
        chapter_pre_len--;
    }
    memcpy(chapter_pre, url->path, path_len);
    memcpy(chapter_pre + path_len, "/page/", 6);
    chapter_pre_len--; // remove the \0

    status = lxb_dom_elements_by_attr_contain (
        lxb_dom_interface_element(document->body), collection,
        "href", 4,
        chapter_pre, chapter_pre_len - 1,
        true
    ); 

    if (status != LXB_STATUS_OK) {
        err = E_WC_FIND_CHAPTER_PAGES;
        goto cleanup;
    };

    lxb_dom_element_t *chapter_anchor;
    lxb_dom_attr_t *href_attr;

    size_t max_page = 1;

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        chapter_anchor = lxb_dom_collection_element(collection, i);

        href_attr = lxb_dom_element_attr_by_name(chapter_anchor, "href", 4);
        if (href_attr == NULL) continue;        
        
        int pos = memsearch(href_attr->value->data, href_attr->value->length, chapter_pre, chapter_pre_len);

        size_t cursor = pos + chapter_pre_len;

        size_t cur_page = atoll (href_attr->value->data + cursor);
        if (cur_page > max_page)
            max_page = cur_page;
    };

    for (size_t i = 2; i < max_page + 1; i++) 
        imp__push_chapter_page(state, chapter, i);

    imp_wc_comic_easel_chapter_page(state, document, url, chapter);

cleanup:
    free (chapter_pre);
    lxb_dom_collection_destroy(collection, true);
    return err;
}

#define NAV_COLLECTION_FIRST(parent, class, class_len, ptr)                             \
    status = lxb_dom_elements_by_class_name (                                           \
        (parent), collection,                                                           \
        (class), (class_len)                                                            \
    ); if ((status != LXB_STATUS_OK) || (lxb_dom_collection_length(collection) < 0)) {  \
        err = E_WC_FIND_NAV;                                                            \
        goto cleanup;                                                                   \
    };                                                                                  \
    ptr = lxb_dom_collection_element(collection, 0)

imp_wc_err_t imp_wc_comic_easel_scrape (imp_wc_indexer_state_t *state, lxb_html_document_t *document, imp_url_t* url) {
    lxb_status_t status;
    imp_wc_err_t err = E_WC_OK;

    imp_wc_meta_strip_t *strip = hashmap_get(state->content, &(imp_wc_meta_strip_t){ .src_url = url });

    if ((strip != NULL) && (strip->is_scraped > 0))
        return E_WC_OK;

    if (strip == NULL) {
        strip = malloc(sizeof(imp_wc_meta_strip_t));
        imp_wc_meta_strip_init(strip);
    }

    err = imp_wc_meta_process_strip(&state->metadata, strip, document, url);
    if (err != E_WC_OK) 
        return err;

    lxb_dom_collection_t *collection;

    CHK_N_EQ(collection, lxb_dom_collection_make(&document->dom_document, 8), NULL, E_WC_CREATE_COLLECTION);

    lxb_dom_element_t *comic_nav;
    NAV_COLLECTION_FIRST(lxb_dom_interface_element(document->body), "comic_navi", 10, comic_nav);

    const lxb_char_t *comic_nav_tag = lxb_dom_element_qualified_name(comic_nav, NULL);

    if (strcmp(comic_nav_tag, "table")) {
        err = E_WC_FIND_NAV;
        goto cleanup;
    };

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *nav_next;
    NAV_COLLECTION_FIRST(comic_nav, "navi-next", 9, nav_next);

    const lxb_char_t *nav_next_class_list = lxb_dom_element_class(nav_next, NULL);

    int has_member;
    has_member = memsearch(nav_next_class_list, strlen(nav_next_class_list), "navi-void", 9);

    if (lxb_dom_element_qualified_name(nav_next, NULL)[0] != 'a') {
        if ((has_member > -1)) {
            // This is the last page!!
            strip->next_url = NULL;
        } else {
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    } else {
        lxb_dom_attr_t *href = lxb_dom_element_attr_by_name(nav_next, "href", 4);
        if (href == NULL)
            goto err_next;
        strip->next_url = imp_parse_url(href->value->data, href->value->length);
        if (strip->next_url == NULL) {
err_next:
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    }

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *nav_prev;
    NAV_COLLECTION_FIRST(comic_nav, "navi-prev", 9, nav_prev);

    const lxb_char_t *nav_prev_class_list = lxb_dom_element_class(nav_prev, NULL);

    has_member = memsearch(nav_prev_class_list, strlen(nav_prev_class_list), "navi-void", 9);

    if (lxb_dom_element_qualified_name(nav_prev, NULL)[0] != 'a') {
        if ((has_member > -1)) {
            // This is the first page!!
            strip->previous_url = NULL;
        } else {
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    } else {
        lxb_dom_attr_t *href = lxb_dom_element_attr_by_name(nav_prev, "href", 4);
        if (href == NULL)
            goto err_prev;
        strip->previous_url = imp_parse_url(href->value->data, href->value->length);
        if (strip->previous_url == NULL) {
err_prev:
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    }

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *img_container;

    status = lxb_dom_elements_by_attr (
        lxb_dom_interface_element(document->body), collection,
        "id", 2,
        "comic", 5,
        true
    );
    
    if ((status != LXB_STATUS_OK) || (lxb_dom_collection_length(collection) < 0)) {
        err = E_WC_FIND_STRIP;
        goto cleanup;
    };

    img_container = lxb_dom_collection_element(collection, 0);

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *strip_img;

    status = lxb_dom_elements_by_tag_name (
        img_container, collection,
        "img", 3
    );
    
    if ((status != LXB_STATUS_OK) || (lxb_dom_collection_length(collection) < 0)) {
        err = E_WC_FIND_STRIP;
        goto cleanup;
    };

    strip_img = lxb_dom_collection_element(collection, 0);

    lxb_dom_attr_t *strip_src = lxb_dom_element_attr_by_name(strip_img, "src", 3);

    if (strip_src == NULL) {
        err = E_WC_FIND_STRIP;
        goto cleanup;
    };

    imp_url_t *prev_src = strip->img_url;
    
    strip->img_url = imp_parse_url(
    strip_src->value->data, 
    strip_src->value->length);

    if (strip->img_url == NULL)
        strip->img_url = prev_src;
    else if (prev_src != NULL)
        imp_url_free(prev_src); 

    lxb_dom_attr_t *strip_alt = lxb_dom_element_attr_by_name(strip_img, "alt", 3);

    if ((strip_alt != NULL) && (strip->img_desc.priority < 2)) {
        strip->img_desc.priority = 2;
        strip->img_desc.data.base = realloc(strip->img_desc.data.base, strip_alt->value->length);
        memcpy(strip->img_desc.data.base, strip_alt->value->data, strip_alt->value->length);
        strip->img_desc.data.len = strip_alt->value->length;
    };

    // TODO NULL + errs
    hashmap_set(state->content, strip);

    imp_wc_download_image (state, strip);

cleanup:
    lxb_dom_collection_destroy(collection, true);
    return err;
};

static void imp__chapter_dl_cb (imp_http_worker_t *worker, imp_http_pool_t *pool) {
    imp_wc_indexer_state_t *state = worker->last_request->data;
    imp_wc_meta_chapter_t *chap = worker->last_request_data;
    lxb_status_t status;
    lxb_html_document_t *document;

    document = lxb_html_document_create();
    if (document == NULL)
        goto fail;

    status = lxb_html_document_parse(document, worker->last_response.base, worker->last_response.len);
    if (status != LXB_STATUS_OK)
        goto fail;

    imp_wc_comic_easel_chapter (state, document, worker->client.url, chap);

fail:
    // TODO FAIL
    lxb_html_document_destroy(document);
}

inline
static void imp__push_chapter (imp_wc_indexer_state_t *state, lxb_dom_attr_t *href_attr, lexbor_str_t *chaptr_name) {
    char *str = calloc(1, href_attr->value->length + 1);
    memcpy(str, href_attr->value->data, href_attr->value->length);
    char *tok;
    tok = strtok(str, "/");
        
    while (tok != NULL) {
        char* p = strtok(NULL, "/");
        if (p == NULL) break;
        tok = p;
    };
    size_t id_len = strlen(tok);
    
    imp_wc_meta_chapter_t *chap;
    chap = hashmap_get(state->chapters, &(imp_wc_meta_chapter_t){ .id = tok, .id_len = id_len });
    
    if (chap == NULL)
        chap = calloc(1, sizeof(imp_wc_meta_chapter_t));

    if (chap->id != NULL)
        free(chap->id);

    chap->id = strdup(tok);
    chap->id_len = id_len;

    if (chap->name != NULL)
        free(chap->name);

    chap->name = calloc(1, chaptr_name->length + 1);
    memcpy(chap->name, chaptr_name->data, chaptr_name->length);

    if (chap->url != NULL)
        imp_url_free(chap->url);

    chap->url = imp_parse_url(href_attr->value->data, href_attr->value->length);

    hashmap_set(state->chapters, chap);

    free(str);

    // now pull it :-)
    if (chap->url == NULL) return;
    imp_http_worker_request_t *req = malloc(sizeof(imp_http_worker_request_t));
    memset(req, 0, sizeof(imp_http_worker_request_t));

    req->url = imp_url_dup(chap->url);

    imp_http_request_t *http_req = imp_http_request_init("GET");
    
    imp_http_pool_default_headers(&http_req->headers);

    http_req->data = state;

    req->request = http_req;

    req->data = chap;

    req->on_response = imp__chapter_dl_cb;

    imp_http_pool_request(&state->pool, req);
}

imp_wc_err_t imp_wc_comic_easel_crawl (imp_wc_indexer_state_t *state, lxb_html_document_t *document, imp_url_t* url) {
    lxb_status_t status;
    imp_wc_err_t err = E_WC_OK;

    lxb_dom_collection_t *collection;

    CHK_N_EQ(collection, lxb_dom_collection_make(&document->dom_document, 128), NULL, E_WC_CREATE_COLLECTION);

    status = lxb_dom_elements_by_attr_contain (
        lxb_dom_interface_element(document->body), collection,
        "value", 5,
        "/chapter/", 9,
        true
    );

    if (status != LXB_STATUS_OK) {
        err = E_WC_FIND_META;
        goto cleanup;
    };

    lxb_dom_element_t *chapter_anchor;
    lxb_dom_attr_t *href_attr;
    lexbor_str_t *chaptr_name;

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        chapter_anchor = lxb_dom_collection_element(collection, i);

        href_attr = lxb_dom_element_attr_by_name(chapter_anchor, "value", 5);

        if (href_attr == NULL) continue;

        DUMP_ATTRIBUTE(href_attr, "CHAPTER");
        chaptr_name = imp_node_text(chapter_anchor);

        if (chaptr_name == NULL) continue;
        
        imp__push_chapter(state, href_attr, chaptr_name);
    };

    if (lxb_dom_collection_length(collection) == 0) {
        // try using a[tag=href] instead.
        lxb_dom_collection_clean(collection);

        status = lxb_dom_elements_by_attr_contain (
            lxb_dom_interface_element(document->body), collection,
            "href", 4,
            "/chapter/", 9,
            true
        );

        if (status != LXB_STATUS_OK) {
            err = E_WC_FIND_META;
            goto cleanup;
        };

        for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
            chapter_anchor = lxb_dom_collection_element(collection, i);

            href_attr = lxb_dom_element_attr_by_name(chapter_anchor, "href", 4);

            if (href_attr == NULL) continue;

            DUMP_ATTRIBUTE(href_attr, "CHAPTER");
            chaptr_name = imp_node_text(chapter_anchor);

            if (chaptr_name == NULL) continue;
            
            imp__push_chapter(state, href_attr, chaptr_name);
        };
    }

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *comic_nav;
    NAV_COLLECTION_FIRST(lxb_dom_interface_element(document->body), "comic_navi", 10, comic_nav);

    const lxb_char_t *comic_nav_tag = lxb_dom_element_qualified_name(comic_nav, NULL);

    if (strcmp(comic_nav_tag, "table")) {
        err = E_WC_FIND_NAV;
        goto cleanup;
    };

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *nav_first;
    NAV_COLLECTION_FIRST(comic_nav, "navi-first", 10, nav_first);

    const lxb_char_t *nav_first_class_list = lxb_dom_element_class(nav_first, NULL);

    int has_member;
    has_member = memsearch(nav_first_class_list, strlen(nav_first_class_list), "navi-void", 9);

    if (lxb_dom_element_qualified_name(nav_first, NULL)[0] != 'a') {
        if ((has_member > -1)) {
            // This is the first page!!
            state->metadata.first_url = imp_url_dup(url);
        } else {
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    } else {
        lxb_dom_attr_t *href = lxb_dom_element_attr_by_name(nav_first, "href", 4);
        if (href == NULL)
            goto err_first;
        state->metadata.first_url = imp_parse_url(href->value->data, href->value->length);
        if (state->metadata.first_url == NULL) {
err_first:
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    }

    printf(">>> FIRST PAGE: %s\n", state->metadata.first_url->path);

    lxb_dom_collection_clean(collection);

    lxb_dom_element_t *nav_last;
    NAV_COLLECTION_FIRST(comic_nav, "navi-last", 9, nav_last);

    const lxb_char_t *nav_last_class_list = lxb_dom_element_class(nav_last, NULL);

    has_member = memsearch(nav_last_class_list, strlen(nav_last_class_list), "navi-void", 9);

    if (lxb_dom_element_qualified_name(nav_last, NULL)[0] != 'a') {
        if ((has_member > -1)) {
            // This is the last page!!
            state->metadata.last_url = imp_url_dup(url);
        } else {
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    } else {
        lxb_dom_attr_t *href = lxb_dom_element_attr_by_name(nav_last, "href", 4);
        if (href == NULL)
            goto err_last;
        state->metadata.last_url = imp_parse_url(href->value->data, href->value->length);
        if (state->metadata.last_url == NULL) {
err_last:
            err = E_WC_PARSE_NAV;
            goto cleanup;
        }
    }

    printf(">>> LAST PAGE: %s\n", state->metadata.last_url->path);
    // since we already have fetched the last page we can process it so long
    imp_wc_comic_easel_scrape(state, document, url);
cleanup:
    lxb_dom_collection_destroy(collection, true);
    return err;
}

#undef NAV_COLLECTION_FIRST
