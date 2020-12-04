#ifndef IMP_RI_MATCH_H
#define IMP_RI_MATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "img-panda/db.h"

// Structure hosting all the C++ classes and matrices for feature detection
typedef struct imp_ri_state_s {
    imp_db_t *db;
    size_t max_area;
    void *data; // C++ Internal data
} imp_ri_state_t;

typedef struct imp_ri_match_pair_s {
    size_t id;
    size_t freq;
} imp_ri_match_pair_t;

typedef struct imp_ri_matches_s {
    imp_ri_match_pair_t *idx;
    size_t len;
} imp_ri_matches_t;

/// State
/**
 * Allocate and create new default state.
 * Free state initialized with this method with imp_ri_state_free
 * Returns 0 on malloc error and 1 elsewise
 */
int imp_ri_state_init (imp_ri_state_t *state);
/**
 * Free the state
 */
void imp_ri_state_free (imp_ri_state_t *state);
/** 
 * Returns 0 on error and 1 on success - if error you MUST call imp_ri_state_verify_integrity
 */
int imp_ri_state_load (imp_ri_state_t *state);
int imp_ri_state_verify_integrity (imp_ri_state_t *state);

/// Matching
/** 
 * Returns NULL on error or no results
 * Always free with imp_ri_match_free
 */
imp_ri_matches_t *imp_ri_match_img (imp_ri_state_t *state, imp_buf_t *img, size_t k);
/**
 * Free results of imp_ri_matches_t*
 */
void imp_ri_match_free (imp_ri_matches_t *matches);
/**
 * Get the image uri from id
 */
const char* imp_ri_get_uri_from_id (imp_ri_state_t *state, size_t id);

/// Appending
/** 
 * Returns NULL on error - if error you MUST call imp_ri_state_verify_integrity
 */
int imp_ri_add_img (imp_ri_state_t *state, imp_buf_t *img, const char *uri);

#define imp_ri_set_db(x, y) x->db = y

#ifdef __cplusplus
}
#endif

#endif