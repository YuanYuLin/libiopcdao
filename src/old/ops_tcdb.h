#ifndef OPS_TCDB_H
#define OPS_TCDB_H

#define DB_PATH_DEFAULT_TCDB	"/tmp/tcdb.db"

struct ops_db_t *get_tcdb_instance();
void del_tcdb_instance();
#endif
