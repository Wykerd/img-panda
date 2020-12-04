#include "img-panda/reverse_image/db.hpp"
#include <sqlite3.h>
#include <iostream>

#define CHECK_BIND_OK(x) rc = x; if (rc != SQLITE_OK) return rc;

int imp_ri_db_sqlite_add_img (imp_db_t *db, cv::Mat &desc, const char* uri) {
    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_prepare((sqlite3*)db->conn, "REPLACE INTO images (uri, descriptors) VALUES (?, ?)", -1, &stmt, 0);

    size_t len;
    char* blob = imp_ri_mat_to_blob(desc, len);
    if (blob == NULL) return 0;

    CHECK_BIND_OK(sqlite3_bind_text(stmt, 1, uri, -1, NULL));
    CHECK_BIND_OK(sqlite3_bind_blob64(stmt, 2, blob, len, imp_ri_blob_free));

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc == SQLITE_OK;
}

int imp_ri_db_sqlite_get_imgs (imp_db_t *db, imp_ri_imgs_t *imgs) {
    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_prepare((sqlite3*)db->conn, "SELECT id, uri, descriptors FROM images", 39, &stmt, 0);

    if (rc != SQLITE_OK) 
        return rc;

    rc = sqlite3_step(stmt);

    while (rc == SQLITE_ROW) {
        size_t id = sqlite3_column_int64(stmt, 0);
        std::string uri = (const char*)sqlite3_column_text(stmt, 1);
        cv::Mat desc = imp_ri_blob_to_mat(sqlite3_column_blob(stmt, 2));
        imgs->matrix.push_back(desc);
        imgs->uris[id] = uri;

        for (int i = imgs->ids_len; i < imgs->matrix.rows; i++)
            imgs->ids[i] = id;
        
        if (id > imgs->max_id)
            imgs->max_id = id;

        imgs->ids_len = imgs->matrix.rows;
        rc = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE; // is success?
}

#undef CHECK_BIND_OK
