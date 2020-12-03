#ifndef IMP_WC_SCRAPE_CE_H
#define IMP_WC_SCRAPE_CE_H

#include "img-panda/comics/identify/metadata.h"
#include "img-panda/comics/index.h"
#include "img-panda/common.h"

#ifdef __cplusplus
extern "C" {
#endif

imp_wc_err_t imp_wc_comic_easel_chapter (imp_wc_indexer_state_t *state, lxb_html_document_t *document, 
                                         imp_url_t* url, imp_wc_meta_chapter_t *chapter);
imp_wc_err_t imp_wc_comic_easel_scrape (imp_wc_indexer_state_t *state, lxb_html_document_t *document, imp_url_t* url);
imp_wc_err_t imp_wc_comic_easel_crawl (imp_wc_indexer_state_t *state, lxb_html_document_t *document, imp_url_t* url);

#ifdef __cplusplus
}
#endif

#endif