#include "img-panda/parser.h"
#include "img-panda/common.h"
#include <lexbor/html/parser.h>
#include <lexbor/dom/interfaces/element.h>

int parse_comic_content (lxb_char_t *html, size_t html_len) {
    lxb_status_t status;
    lxb_dom_element_t *body;
    lxb_html_document_t *document;
    lxb_dom_collection_t *collection;

    CHK_N_EQ(document, lxb_html_document_create(), NULL, "Could not create DOM");

    CHK_EQ(status, lxb_html_document_parse(document, html, html_len), LXB_STATUS_OK, "Could not parse document");

    body = lxb_dom_interface_element(document->body);

    CHK_N_EQ(collection, lxb_dom_collection_make(&document->dom_document, 1), NULL, "Could not make DOMCollection");

    CHK_EQ(status, lxb_dom_elements_by_attr (
        body, collection,
        (const lxb_char_t *) "id", 2,
        (const lxb_char_t *) "comic", 5,
        true
    ), LXB_STATUS_OK, "Could not find comic attr");

    lxb_dom_element_t *element;
    lxb_dom_element_t *comic_image;
    lxb_dom_attr_t *comic_src;
    lxb_dom_collection_t *images;

    if (lxb_dom_collection_length(collection) > 0) {
        element = lxb_dom_collection_element(collection, 0);

        images = lxb_dom_collection_make(&document->dom_document, 1);
        if (images == NULL) {
            puts("Failed to create image Collection object");
            return 1;
        };

        status = lxb_dom_elements_by_tag_name(
            element, images,
            (const lxb_char_t *) "img", 3
        );

        if (status != LXB_STATUS_OK) {
            puts("Failed to get elements by tag 'img'");
            return 1;
        }

        if (lxb_dom_collection_length(images) > 0) {
            comic_image = lxb_dom_collection_element(images, 0);

            comic_src = lxb_dom_element_attr_by_name(comic_image, "src", 3);

            if (comic_src)
                fwrite (comic_src->value->data, comic_src->value->length, sizeof(lxb_char_t), stdout);
            
            putc('\n', stdout);
        }

        lxb_dom_collection_destroy(images, true);
    };

    lxb_dom_collection_destroy(collection, true);
    lxb_html_document_destroy(document);

    return 0;
}