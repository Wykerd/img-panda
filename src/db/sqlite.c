#include <sqlite3.h>
#include "img-panda/db.h"

static const char *imp__sqlite_create_tables[] = {
    "CREATE TABLE IF NOT EXISTS images ("
        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        "uri TEXT NOT NULL UNIQUE,"
        "descriptors BLOB NOT NULL"
    ")",
};

int imp_db_sqlite_connect (imp_db_t *db, const char *filename) {
    db->type = IMP_RI_DB_SQLITE;
    return sqlite3_open(filename, (sqlite3**)&db->conn);
};

int imp_db_sqlite_init (imp_db_t *db) {
    for (size_t i = 0; i < (sizeof(imp__sqlite_create_tables) / sizeof(char*)); i++) {
        sqlite3_stmt *stmt;
        int rc;

        rc = sqlite3_prepare(db->conn, imp__sqlite_create_tables[i], -1, &stmt, 0);
        
        if (rc != SQLITE_OK)
            return rc;

        rc = sqlite3_step(stmt);

        sqlite3_finalize(stmt);

        if (rc != SQLITE_OK)
            return rc;
    };
    return SQLITE_OK;
};

