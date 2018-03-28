#ifndef OPS_BDB_H
#define OPS_BDB_H

#define DB_PATH_DEFAULT_BDB	"/tmp/bdb.db"

struct ops_db_t *get_bdb_instance();
void del_bdb_instance();
#endif
