#include <db.h>

#include "ops_log.h"
#include "ops_misc.h"
#include "ops_dbutil.h"
#include "ops_db.h"
#include "ops_bdb.h"

#define DB_PATH_LEN	32
static uint8_t db_path[DB_PATH_LEN] = { 0 };

static uint32_t get_value(uint8_t * key, uint8_t * val, uint8_t * type)
{
	int ret = -1;
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	uint32_t size = 0;
	struct ops_dbutil_t *dbutil = get_dbutil_instance();
	struct ops_log_t *log = get_log_instance();

	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   db_strerror(ret));
		return size;
	}

	if ((ret =
	     dbp->open(dbp, NULL, db_path, NULL, DB_BTREE, DB_CREATE,
		       0664)) != 0) {
		log->error(0x01, "(%s)db_open error[%s]\n", __func__,
			   db_strerror(ret));
		return size;
	}

	memset(&dbkey, 0, sizeof(dbkey));
	memset(&dbval, 0, sizeof(dbval));
	dbkey.data = key;
	dbkey.size = strlen(key);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	dbp->close(dbp, 0);
	//memcpy(val, dbval.data, dbval.size);
	//size = dbval.size;
	size = dbutil->parse_value(val, dbval.data, dbval.size, type);

	log->debug(0x01, "get_value():[%d, %s,%d:%s]\n", dbkey.size, dbkey.data,
		   dbval.size, dbval.data);
	log->debug(0x01, "get_value():(%d, %s, %d, %s)\n", strlen(key), key,
		   strlen(val), val);

	return size;
}

#define SIZE_STR_MAX	2048
static void set_value(uint8_t * key, uint8_t * val, uint8_t * type)
{
	int ret;
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	uint32_t val_len = 0;
	uint8_t format_string[SIZE_STR_MAX] = { 0 };
	struct ops_dbutil_t *dbutil = get_dbutil_instance();
	struct ops_log_t *log = get_log_instance();

	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   db_strerror(ret));
		return;
	}

	if ((ret =
	     dbp->open(dbp, NULL, db_path, NULL, DB_BTREE, DB_CREATE,
		       0664)) != 0) {
		log->error(0x01, "(%s)db_open error[%s]\n", __func__,
			   db_strerror(ret));
	}

	memset(&dbkey, 0, sizeof(dbkey));
	memset(&dbval, 0, sizeof(dbval));
	memset(&format_string[0], 0, SIZE_STR_MAX);

	dbkey.data = key;
	dbkey.size = strlen(key);

	dbval.data = &format_string[0];
	//dbval.size = strlen(val);
	dbval.size = dbutil->format_value(dbval.data, val, val_len, type);

	log->debug(0x01, "set_value(): %d, %s, %d, %s\n", strlen(key), key,
		   strlen(val), val);

	dbp->put(dbp, NULL, &dbkey, &dbval, 0);

	dbp->close(dbp, 0);
}

static void init(void)
{
	struct ops_misc_t *misc = get_misc_instance();
	if (misc->get_dbpath(&db_path[0]) < 0) {
		strcpy(&db_path[0], DB_PATH_DEFAULT_BDB);
	}
}

static uint8_t get_boolean(uint8_t * key)
{
	uint8_t val = 0;
	uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	size = get_value(key, &val, DB_DATATYPE_BOOLEAN);
	log->debug(0x01, "(%s-%d)key:%s, val:%d, size:%ld\n", __func__,
		   __LINE__, key, val, size);

	return val;
}

static void set_boolean(uint8_t * key, uint8_t val)
{
	set_value(key, &val, DB_DATATYPE_BOOLEAN);
}

static uint32_t get_uint32(uint8_t * key)
{
	uint32_t val = 0;
	uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	size = get_value(key, (uint8_t *) & val, DB_DATATYPE_UINT32);
	log->debug(0x01, "(%s-%d)key:%s, val:%ld, size:%ld\n", __func__,
		   __LINE__, key, val, size);

	return val;
}

static void set_uint32(uint8_t * key, uint32_t val)
{
	set_value(key, (uint8_t *) & val, DB_DATATYPE_UINT32);
}

static uint32_t get_string(uint8_t * key, uint8_t * val)
{
	uint32_t size = 0;
	size = get_value(key, val, DB_DATATYPE_STRING);
	return size;
}

static void set_string(uint8_t * key, uint8_t * val)
{
	set_value(key, val, DB_DATATYPE_STRING);
}

static struct ops_db_t *obj;
struct ops_db_t *get_bdb_instance()
{
	if (!obj) {
		obj = malloc(sizeof(struct ops_db_t));
		obj->init = init;
		obj->get_boolean = get_boolean;
		obj->set_boolean = set_boolean;
		obj->get_uint32 = get_uint32;
		obj->set_uint32 = set_uint32;
		obj->get_string = get_string;
		obj->set_string = set_string;
	}

	return obj;
}

void del_bdb_instance()
{
	if (obj)
		free(obj);
}
