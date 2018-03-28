#ifndef OPS_TTDB_H
#define OPS_TTDB_H

#define DB_PATH_DEFAULT_TTDB	"/tmp/ttdb.db"

struct ops_db_t *get_ttdb_instance();
void del_ttdb_instance();
#endif
