#ifndef IMP_ERR_H
#define IMP_ERR_H
#ifdef __cplusplus
extern "C" {
#endif
/*
const char *imp_wc_err_name[] = {
    "Success",
    "Failed to create DOM",
    "Failed to parse DOM",
    "Failed to make DOMCollection",
    "Failed to locate comic container",
    "Failed to locate comic img element",
    "Failed to locate comic img src attribute"
};
*/
typedef enum imp_wc_err {
    E_WC_OK,
    E_WC_CREATE_DOM,
    E_WC_PARSE_DOM,
    E_WC_CREATE_COLLECTION,
    E_WC_FIND_COMIC_CONTAINER,
    E_WC_FIND_COMIC_ELEMENT,
    E_WC_FIND_COMIC_SRC,
    E_WC_FIND_META,
    E_WC_FIND_NAV,
    E_WC_PARSE_NAV,
    E_WC_FIND_HEADER,
    E_WC_FIND_LOGO,
    E_WC_FIND_STRIP,
    E_WC_FIND_CHAPTER_PAGES,
} imp_wc_err_t;

#ifdef __cplusplus
}
#endif
#endif