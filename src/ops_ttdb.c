#include <tcrdb.h>

#include "ops_log.h"
#include "ops_misc.h"
#include "ops_dbutil.h"
#include "ops_db.h"
#include "ops_ttdb.h"

#define DB_PATH_LEN 32
static uint8_t db_path[DB_PATH_LEN] = { 0 };

static uint32_t get_value(uint8_t * key, uint8_t * val, uint8_t * type)
{
	uint32_t size = 0;
	int ecode = 0;
	char *value = NULL;
	TCRDB *rdb;
	struct ops_dbutil_t *dbutil = get_dbutil_instance();
	struct ops_log_t *log = get_log_instance();

	rdb = tcrdbnew();

	if (!tcrdbopen(rdb, "localhost", 1978)) {
		ecode = tcrdbecode(rdb);
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   tcrdberrmsg(ecode));
		return size;
	}

	value = tcrdbget2(rdb, key);
	if (value) {
		size = dbutil->parse_value(val, value, strlen(value), type);
		free(value);
	} else {
		//copy default value to value
		log->error(0x01, "(%s-%d)load default value\n", __func__,
			   __LINE__);
	}

	if (!tcrdbclose(rdb)) {
		ecode = tcrdbecode(rdb);
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   tcrdberrmsg(ecode));
		return size;
	}
	tcrdbdel(rdb);
	return size;
}

#define SIZE_STR_MAX	2048
static uint32_t set_value(uint8_t * key, uint8_t * val, uint8_t * type)
{
	uint32_t size = 0;
	uint32_t val_len = 0;
	int ecode = 0;
	TCRDB *rdb;
	uint8_t format_string[SIZE_STR_MAX] = { 0 };
	struct ops_dbutil_t *dbutil = get_dbutil_instance();
	struct ops_log_t *log = get_log_instance();

	rdb = tcrdbnew();

	if (!tcrdbopen(rdb, "localhost", 1978)) {
		ecode = tcrdbecode(rdb);
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   tcrdberrmsg(ecode));
		return size;
	}

	size = dbutil->format_value(&format_string[0], val, val_len, type);

	tcrdbput2(rdb, key, &format_string[0]);

	if (!tcrdbclose(rdb)) {
		ecode = tcrdbecode(rdb);
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   tcrdberrmsg(ecode));
		return size;
	}
	tcrdbdel(rdb);
}

static void init(void)
{
	struct ops_misc_t *misc = get_misc_instance();
	if (misc->get_dbpath(&db_path[0]) < 0) {
		strcpy(&db_path[0], DB_PATH_DEFAULT_TTDB);
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

static struct ops_db_t *obj = NULL;
struct ops_db_t *get_ttdb_instance()
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

void del_ttdb_instance()
{
	if (obj)
		free(obj);
}
