/* ---
 * Database abstraction layer for image database
 * ---
 * Schema:
 * id | uri | keypoints | descriptors 
 */

#ifndef IMP_DB_H
#define IMP_DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef enum imp_db_con {
    IMP_RI_DB_SQLITE = 0,
    IMP_RI_DB_POSTGRES = 1
} imp_db_con_t;

typedef struct imp_db_s {
    void* conn;
    imp_db_con_t type;
} imp_db_t;

int imp_db_sqlite_connect (imp_db_t *db, const char *filename);
// int imp_db_pq_connect (imp_db_t *db, const char *conninfo);

int imp_db_sqlite_init (imp_db_t *db);

#ifdef __cplusplus
}
#endif

#endif