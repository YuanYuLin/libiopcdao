#include "ops_misc.h"
#include "ops_db.h"
#include "ops_bdb.h"
#include "ops_tcdb.h"
#include "ops_ttdb.h"
#include "ops_dummydb.h"

static struct ops_db_t *obj = NULL;
struct ops_db_t *get_db_instance()
{
	uint32_t db_type = DB_TYPE_DUMMY;
	struct ops_db_t *dao = NULL;
	struct ops_misc_t *misc = get_misc_instance();
	if (!obj) {
		obj = malloc(sizeof(struct ops_db_t));
	}
	if (misc->get_dbtype(&db_type) < 0) {
		db_type = DB_TYPE_DUMMY;
	}
	switch (db_type) {
	case DB_TYPE_BDB:
		dao = get_bdb_instance();
		break;
	case DB_TYPE_TCDB:
		dao = get_tcdb_instance();
		break;
	case DB_TYPE_TTDB:
		dao = get_ttdb_instance();
		break;
	default:
		dao = get_dummydb_instance();
		break;
	}
	obj->init = dao->init;
	obj->get_boolean = dao->get_boolean;
	obj->set_boolean = dao->set_boolean;
	obj->get_uint32 = dao->get_uint32;
	obj->set_uint32 = dao->set_uint32;
	obj->get_string = dao->get_string;
	obj->set_string = dao->set_string;
	return obj;
}

void del_db_instance()
{
	if (obj)
		free(obj);
}
