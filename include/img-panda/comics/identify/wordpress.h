#ifndef IMP_WC_WORDPRESS_H
#define IMP_WC_WORDPRESS_H
#ifdef __cplusplus
extern "C" {
#endif

#include "img-panda/comics/identify/metadata.h"

int imp_parse_wordpress_site_info_json (imp_wc_meta_index_t *page, imp_buf_t *json);

#ifdef __cplusplus
}
#endif
#endif