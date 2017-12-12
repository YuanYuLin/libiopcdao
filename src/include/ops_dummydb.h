#ifndef OPS_DUMMYDB_H
#define OPS_DUMMYDB_H

#define DB_PATH_DEFAULT_DUMMYDB	"/tmp/dummydb.db"

struct ops_db_t *get_dummydb_instance();
void del_dummydb_instance();
#endif
