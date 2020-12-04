#ifndef IMP_RI_DB_H
#define IMP_RI_DB_H

#include "img-panda/db.h"
#include <opencv2/core.hpp>
#include <unordered_map>

#define MAT_BLOB_HEADER_LEN (sizeof(int) * 4)

typedef struct imp_ri_kpdesc_s {
    cv::Mat desc;
    std::vector<cv::KeyPoint> kp;
} imp_ri_kpdesc_t;

typedef struct imp_ri_imgs_s {
    cv::Mat matrix;
    std::unordered_map<size_t, std::string> uris;
    std::unordered_map<size_t, size_t> ids;
    size_t ids_len;
    size_t max_id;
} imp_ri_imgs_t;

typedef struct imp_ri_distributed_imgs_s {
    std::vector<imp_ri_imgs_t> imgs;
    size_t current_index;
    size_t size;
    size_t max_id;
} imp_ri_distributed_imgs_t;

inline
void imp_ri_blob_free (void *src) {
    free(src);
};

char *imp_ri_mat_to_blob (cv::Mat &src, size_t &len);
cv::Mat imp_ri_blob_to_mat (const void* src);

typedef struct imp_ri_db_funcs_s {
    int (*add_img) (imp_db_t *db, cv::Mat &desc, const char* uri);
    int (*get_imgs) (imp_db_t *db, imp_ri_imgs_t *imgs);
} imp_ri_db_funcs_t;


int imp_ri_db_sqlite_add_img (imp_db_t *db, cv::Mat &desc, const char* uri);
int imp_ri_db_sqlite_get_imgs (imp_db_t *db, imp_ri_imgs_t *imgs);

const imp_ri_db_funcs_t imp_ri_db_default_funcs[] = {
    {
        .add_img = imp_ri_db_sqlite_add_img,
        .get_imgs = imp_ri_db_sqlite_get_imgs
    }
};

#define imp_ri_db_add_img(db, desc, uri) imp_ri_db_default_funcs[db->type].add_img(db, desc, uri)

#define imp_ri_db_get_imgs(db, imgs) imp_ri_db_default_funcs[db->type].get_imgs(db, imgs)

#endif