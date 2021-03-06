#ifndef IMP_WC_META_H
#define IMP_WC_META_H
#ifdef __cplusplus
extern "C" {
#endif

#include "img-panda/comics/err.h"
#include "img-panda/common.h"
#include "img-panda/parsers/url.h"
#include <stddef.h>
#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>

typedef enum imp_meta_flag {
    META_NONE,
    META_CMS_WORDPRESS,
    META_CMS_PL_COMIC_EASEL,
    META_CMS_PL_COMICPRESS,
} imp_meta_flag_t;

typedef struct imp_wc_meta_field_s {
    unsigned int priority;
    imp_meta_flag_t data;
} imp_wc_meta_field_t;

typedef struct imp_wc_meta_content_s {
    unsigned int priority;
    imp_buf_t data;
} imp_wc_meta_content_t;

typedef struct imp_wc_meta_list_s {
    imp_buf_t **list;
    size_t len;
} imp_wc_meta_list_t;

typedef struct imp_wc_meta_index_s {
    imp_wc_meta_field_t   cms;
    imp_wc_meta_field_t   plugin;
    imp_wc_meta_content_t site_title;
    imp_wc_meta_content_t site_desc;
    imp_wc_meta_content_t site_logo;

    imp_url_t *first_url;
    imp_url_t *last_url;
} imp_wc_meta_index_t;

typedef struct imp_wc_meta_chapter_s {
    imp_url_t *url;
    char* id; // although we store id_len this must still be a null terminated c str.
    size_t id_len;
    char *name;
} imp_wc_meta_chapter_t;

typedef struct imp_wc_meta_strip_s {
    imp_wc_meta_content_t img_title;
    imp_wc_meta_content_t img_desc;
    imp_wc_meta_list_t    tags;

    imp_url_t *src_url;

    imp_url_t *img_url;
    imp_url_t *next_url;
    imp_url_t *previous_url;

    imp_wc_meta_chapter_t *chapter;
    int is_scraped;
} imp_wc_meta_strip_t;

inline 
static lexbor_str_t *imp_node_text (lxb_dom_element_t *el) {
    if ((el->node.first_child != NULL) && 
        !lxb_html_node_is_void(el->node.first_child) && 
        (el->node.first_child->type == LXB_DOM_NODE_TYPE_TEXT)) 
    {
        lxb_dom_text_t *text = lxb_dom_interface_text(el->node.first_child);
        lexbor_str_t *data = &text->char_data.data;

        return data;
    } 
    else 
        return NULL;
}

int imp_wc_meta_index_init (imp_wc_meta_index_t *page);
int imp_wc_meta_strip_init (imp_wc_meta_strip_t *page);
void imp_wc_meta_index_free (imp_wc_meta_index_t *page);
void imp_wc_meta_strip_free (imp_wc_meta_strip_t *page);

imp_wc_err_t imp_wc_meta_process_index (imp_wc_meta_index_t *page, lxb_html_document_t *document, imp_url_t* url);

imp_wc_err_t imp_wc_meta_process_strip (imp_wc_meta_index_t *index, imp_wc_meta_strip_t *page,  lxb_html_document_t *document, imp_url_t* url);

#ifdef __cplusplus
}
#endif
#endif