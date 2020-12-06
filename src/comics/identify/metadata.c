// TODO malloc null checks

#include "img-panda/comics/identify/metadata.h"
#include "lexbor/html/html.h"
#include <string.h>

#define DUMP_IDENTIFIER(x) DUMP_ATTRIBUTE(content_attr, x)

int imp_wc_meta_index_init (imp_wc_meta_index_t *page) {
    memset(page, 0, sizeof(imp_wc_meta_index_t));
    page->site_title.data.base = malloc(1);
    page->site_desc.data.base = malloc(1);
    page->site_logo.data.base = malloc(1);

    return 1;
};

void imp_wc_meta_strip_free (imp_wc_meta_strip_t *page) {
    imp_url_free(page->src_url);
    imp_url_free(page->img_url);
    imp_url_free(page->next_url);
    imp_url_free(page->previous_url);
    free(page->img_title.data.base);
    free(page->img_desc.data.base);
}

int imp_wc_meta_strip_init (imp_wc_meta_strip_t *page) {
    memset(page, 0, sizeof(imp_wc_meta_strip_t));
    page->img_title.data.base = malloc(1);
    page->img_desc.data.base = malloc(1);
    
    return 1;
};

void imp_wc_meta_index_free (imp_wc_meta_index_t *page) {
    free(page->site_title.data.base);
    free(page->site_desc.data.base);
    free(page->site_logo.data.base);

    if (page->first_url != NULL)
        imp_url_free(page->first_url);
    if (page->last_url != NULL)
        imp_url_free(page->last_url);
};

inline
static imp_wc_err_t imp__comic_logo_locate (imp_wc_meta_index_t *page, lxb_html_document_t *document, imp_url_t* url) {
    puts(">>> IDENTIFYING LOGO");
    lxb_status_t status;
    lxb_dom_collection_t *headers;

    CHK_N_EQ(headers, lxb_dom_collection_make(&document->dom_document, 16), NULL, E_WC_CREATE_COLLECTION);

    CHK_EQ_CB(status, lxb_dom_elements_by_tag_name (
        lxb_dom_interface_element(document->body), headers,
        "header", 6
    ), LXB_STATUS_OK, E_WC_FIND_HEADER, lxb_dom_collection_destroy(headers, true));

    lxb_dom_element_t *header;
    lxb_dom_collection_t *imgs;
    lxb_dom_element_t *img;
    lxb_dom_attr_t *src_attr;

    CHK_N_EQ_CB(imgs, lxb_dom_collection_make(&document->dom_document, 16), NULL, E_WC_CREATE_COLLECTION, lxb_dom_collection_destroy(headers, true));

    for (size_t i = 0; i < lxb_dom_collection_length(headers); i++) {
        header = lxb_dom_collection_element(headers, i);

        // Check for logo image
        CHK_EQ_CB(status, lxb_dom_elements_by_tag_name(
            header, imgs,
            "img", 3
        ), LXB_STATUS_OK, E_WC_FIND_LOGO, lxb_dom_collection_destroy(headers, true); lxb_dom_collection_destroy(imgs, true));

        for (size_t i = 0; i < lxb_dom_collection_length(imgs); i++) {
            img = lxb_dom_collection_element(imgs, i);
            src_attr = lxb_dom_element_attr_by_name(img, "src", 3);
            if (src_attr == NULL) continue;
            if (memsearch(src_attr->value->data, src_attr->value->length, url->host, strlen(url->host))> -1) {
                DUMP_ATTRIBUTE(src_attr, "Logo");
                goto exit;
            }
        };
    }

exit:
    lxb_dom_collection_destroy(headers, true);
    lxb_dom_collection_destroy(imgs, true);

    return E_WC_OK;
}

imp_wc_err_t imp_wc_meta_process_index (imp_wc_meta_index_t *page, lxb_html_document_t *document, imp_url_t* url) {
    lxb_status_t status;
    lxb_dom_collection_t *metas;

    CHK_N_EQ(metas, lxb_dom_collection_make(&document->dom_document, 16), NULL, E_WC_CREATE_COLLECTION);

    CHK_EQ_CB(status, lxb_dom_elements_by_tag_name (
        lxb_dom_interface_element(document->head), metas,
        "meta", 4
    ), LXB_STATUS_OK, E_WC_FIND_META, lxb_dom_collection_destroy(metas, true));

    lxb_dom_element_t *element;

    lxb_dom_attr_t *name_attr;
    lxb_dom_attr_t *content_attr;
    lxb_dom_attr_t *property_attr;

    for (size_t i = 0; i < lxb_dom_collection_length(metas); i++) {
        element = lxb_dom_collection_element(metas, i);

        name_attr = lxb_dom_element_attr_by_name(element, "name", 4);
        content_attr = lxb_dom_element_attr_by_name(element, "content", 7);
        property_attr = lxb_dom_element_attr_by_name(element, "property", 8);

        if (content_attr == NULL) continue;

        if (name_attr != NULL) {
            switch (name_attr->value->length)
            {
            case 9:
                {
                    if ((content_attr->value->length > 8) && !memcmp(name_attr->value->data, "generator", 9)) {
                        DUMP_IDENTIFIER("CMS");
                        if (!memcmp(content_attr->value->data, "WordPress", 9)) {
                            if (page->cms.priority < 1) {
                                page->cms.priority = 1;
                                page->cms.data = META_CMS_WORDPRESS;
                            }
                        }
                    }
                }
                break;

            case 11:
                {
                    if (!memcmp(name_attr->value->data, "Comic-Easel", 11)) {
                        if (page->plugin.priority < 1) {
                            DUMP_IDENTIFIER("Comic Plugin: Comic-Easel");
                            page->plugin.priority = 1;
                            page->plugin.data = META_CMS_PL_COMIC_EASEL;
                        }
                    }
                }
                break;

            case 10:
                {
                    if (!memcmp(name_attr->value->data, "ComicPress", 10)) {
                        if (page->plugin.priority < 2) {
                            DUMP_IDENTIFIER("Comic Plugin: ComicPress");
                            page->plugin.priority = 2;
                            page->plugin.data = META_CMS_PL_COMICPRESS;
                        }
                    }
                }
                break;
            }
        }
    };

    lxb_dom_collection_destroy(metas, true);

    imp__comic_logo_locate(page, document, url);

    return E_WC_OK;
}

imp_wc_err_t imp_wc_meta_process_strip (imp_wc_meta_index_t *index, imp_wc_meta_strip_t *page, 
                                        lxb_html_document_t *document, imp_url_t* url) 
{
    lxb_status_t status;
    lxb_dom_collection_t *metas;

    page->src_url = imp_url_dup(url);

    CHK_N_EQ(metas, lxb_dom_collection_make(&document->dom_document, 16), NULL, E_WC_CREATE_COLLECTION);

    CHK_EQ_CB(status, lxb_dom_elements_by_tag_name (
        lxb_dom_interface_element(document->head), metas,
        "meta", 4
    ), LXB_STATUS_OK, E_WC_FIND_META, lxb_dom_collection_destroy(metas, true));

    lxb_dom_element_t *element;

    lxb_dom_attr_t *content_attr;
    lxb_dom_attr_t *property_attr;

    // Metadata processing
    for (size_t i = 0; i < lxb_dom_collection_length(metas); i++) {
        element = lxb_dom_collection_element(metas, i);

        content_attr = lxb_dom_element_attr_by_name(element, "content", 7);
        property_attr = lxb_dom_element_attr_by_name(element, "property", 8);

        if (content_attr == NULL) continue;
        if (property_attr != NULL) {
            // checking for open graph
            if ((property_attr->value->length > 7) && !memcmp(property_attr->value->data, "og:", 3)) {
                switch (property_attr->value->length)
                {
                case 8:
                    {
                        if (!memcmp(property_attr->value->data, "og:image", 8)) {
                            if (page->img_url == NULL) {
                                DUMP_IDENTIFIER("Image URL");
                                page->img_url = imp_parse_url(content_attr->value->data, content_attr->value->length);
                            }
                        } else if (!memcmp(property_attr->value->data, "og:title", 8)) {
                            if (page->img_title.priority < 1) {
                                DUMP_IDENTIFIER("Image Title");
                                page->img_title.priority = 1;
                                page->img_title.data.base = realloc(page->img_title.data.base, content_attr->value->length);
                                memcpy(page->img_title.data.base, content_attr->value->data, content_attr->value->length);
                                page->img_title.data.len = content_attr->value->length;
                            }
                        }
                    }
                    break;

                case 12:
                    {
                        if (!memcmp(property_attr->value->data, "og:image:url", 12)) {
                            if (page->img_url == NULL) {
                                DUMP_IDENTIFIER("Image URL");
                                page->img_url = imp_parse_url(content_attr->value->data, content_attr->value->length);
                            }
                        } else if (!memcmp(property_attr->value->data, "og:image:alt", 12)) {
                            if (page->img_desc.priority < 1) {
                                DUMP_IDENTIFIER("Image Description");
                                page->img_desc.priority = 1;
                                page->img_desc.data.base = realloc(page->img_desc.data.base, content_attr->value->length);
                                memcpy(page->img_desc.data.base, content_attr->value->data, content_attr->value->length);
                                page->img_desc.data.len = content_attr->value->length;
                            }
                        } else if (!memcmp(property_attr->value->data, "og:site_name", 12)) {
                            if (index->site_title.priority < 1) {
                                DUMP_IDENTIFIER("Site Name");
                                index->site_title.priority = 1;
                                index->site_title.data.base = realloc(index->site_title.data.base, content_attr->value->length);
                                memcpy(index->site_title.data.base, content_attr->value->data, content_attr->value->length);
                                index->site_title.data.len = content_attr->value->length;
                            }
                        }
                    }
                    break;

                case 14:
                    {
                        if (!memcmp(property_attr->value->data, "og:description", 14)) {
                            if (page->img_desc.priority < 2) {
                                DUMP_IDENTIFIER("Image Description");
                                page->img_desc.priority = 2;
                                page->img_desc.data.base = realloc(page->img_desc.data.base, content_attr->value->length);
                                memcpy(page->img_desc.data.base, content_attr->value->data, content_attr->value->length);
                                page->img_desc.data.len = content_attr->value->length;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    lxb_dom_collection_destroy(metas, true);

    return E_WC_OK;
}

#undef DUMP_IDENTIFIER
