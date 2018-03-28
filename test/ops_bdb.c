#include <db.h>

#include "ops_log.h"
#include "ops_misc.h"
#include "ops_dbutil.h"
#include "ops_db.h"
#include "ops_bdb.h"
#include "ops_json.h"

#define DB_PATH_LEN	32
static uint8_t db_path[DB_PATH_LEN] = { 0 };

static void init(void)
{
	struct ops_misc_t *misc = get_misc_instance();
	if (misc->get_dbpath(&db_path[0]) < 0) {
		strcpy(&db_path[0], DB_PATH_DEFAULT_BDB);
	}
}

static uint8_t get_boolean(uint8_t* tbl, uint8_t * key)
{
	int ret = -1;
	uint8_t val = 0;
	uint8_t defval = 0;
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	struct ops_json_t* json = get_json_instance();

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
	dbkey.data = tbl;
	dbkey.size = strlen(tbl);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	size = dbval.size;
	json_reader_t* reader = json->create_json_reader(dbval.data);
	val = json->get_json_boolean(reader, key, defval);

	dbp->close(dbp, 0);
	log->debug(0x01, "(%s-%d)key:%s, val:%d, size:%ld\n", __func__,
		   __LINE__, key, val, size);

	return val;
}

static void set_boolean(uint8_t* tbl, uint8_t * key, uint8_t val)
{
	int ret = -1;
	uint8_t defval = (~val)?1:0;
	uint8_t json_str[MAX_DATA_SIZE] = {0};
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	//uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	struct ops_json_t* json = get_json_instance();

	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   db_strerror(ret));
		return ;
	}

	if ((ret =
	     dbp->open(dbp, NULL, db_path, NULL, DB_BTREE, DB_CREATE,
		       0664)) != 0) {
		log->error(0x01, "(%s)db_open error[%s]\n", __func__,
			   db_strerror(ret));
		return ;
	}

	memset(&dbkey, 0, sizeof(dbkey));
	memset(&dbval, 0, sizeof(dbval));
	dbkey.data = tbl;
	dbkey.size = strlen(tbl);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	//size = dbval.size;
	json_reader_t* reader = json->create_json_reader(dbval.data);
	json_writer_t* writer = reader;
	if (val != json->get_json_boolean(reader, key, defval)) {
		json->set_json_boolean(writer, key, val);
		json->out_json_to_bytes(writer, &json_str[0]);

		memset(&dbval, 0, sizeof(dbval));
		dbval.data = &json_str[0];
		dbval.size = strlen(json_str);

		log->debug(0x01, "set_value(): %d, %s, %d, %s\n", strlen(tbl), tbl,
				strlen(json_str), json_str);

		dbp->put(dbp, NULL, &dbkey, &dbval, 0);
	}

	dbp->close(dbp, 0);
}

static uint32_t get_uint32(uint8_t* tbl, uint8_t * key)
{
	int ret = -1;
	uint32_t val = 0;
	uint32_t defval = 0;
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	struct ops_json_t *json = get_json_instance();

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
	dbkey.data = tbl;
	dbkey.size = strlen(tbl);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	size = dbval.size;
	json_reader_t* reader = json->create_json_reader(dbval.data);
	val = json->get_json_int(reader, key, defval);

	dbp->close(dbp, 0);
	log->debug(0x01, "(%s-%d)key:%s, val:%d, size:%ld\n", __func__,
		   __LINE__, key, val, size);

	return val;
}

static void set_uint32(uint8_t* tbl, uint8_t * key, uint32_t val)
{
	int ret = -1;
	uint32_t defval = val + 1;
	uint8_t json_str[MAX_DATA_SIZE] = {0};
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	//uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	struct ops_json_t *json = get_json_instance();

	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   db_strerror(ret));
		return ;
	}

	if ((ret =
	     dbp->open(dbp, NULL, db_path, NULL, DB_BTREE, DB_CREATE,
		       0664)) != 0) {
		log->error(0x01, "(%s)db_open error[%s]\n", __func__,
			   db_strerror(ret));
		return ;
	}

	memset(&dbkey, 0, sizeof(dbkey));
	memset(&dbval, 0, sizeof(dbval));
	dbkey.data = tbl;
	dbkey.size = strlen(tbl);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	//size = dbval.size;
	json_reader_t* reader = json->create_json_reader(dbval.data);
	json_writer_t* writer = reader;
	if (val != json->get_json_int(reader, key, defval)) {
		json->set_json_int(writer, key, val);
		json->out_json_to_bytes(writer, &json_str[0]);

		memset(&dbval, 0, sizeof(dbval));
		dbval.data = &json_str[0];
		dbval.size = strlen(json_str);

		log->debug(0x01, "set_value(): %d, %s, %d, %s\n", strlen(tbl), tbl,
				strlen(json_str), json_str);

		dbp->put(dbp, NULL, &dbkey, &dbval, 0);
	}

	dbp->close(dbp, 0);
}

static uint32_t get_string(uint8_t* tbl, uint8_t * key, uint8_t * val)
{
	int ret = -1;
	uint8_t *defval = NULL;
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	struct ops_json_t* json = get_json_instance();

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
	dbkey.data = tbl;
	dbkey.size = strlen(tbl);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	size = dbval.size;
	json_reader_t* reader = json->create_json_reader(dbval.data);
	val = json->get_json_string(reader, key, defval);

	dbp->close(dbp, 0);
	log->debug(0x01, "(%s-%d)tbl:%s, key:%s, val:%d, size:%ld\n", __func__,
		   __LINE__, tbl, key, val, size);

	return size;
}

static void set_string(uint8_t* tbl, uint8_t * key, uint8_t * val)
{
	int ret = -1;
	uint8_t* defval = NULL;
	uint8_t json_str[MAX_DATA_SIZE] = {0};
	uint8_t* str_ptr = NULL;
	DB *dbp;
	DBT dbkey;
	DBT dbval;
	//uint32_t size = 0;
	struct ops_log_t *log = get_log_instance();
	struct ops_json_t* json = get_json_instance();

	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		log->error(0x01, "(%s)db_create error[%s]\n", __func__,
			   db_strerror(ret));
		return ;
	}

	if ((ret =
	     dbp->open(dbp, NULL, db_path, NULL, DB_BTREE, DB_CREATE,
		       0664)) != 0) {
		log->error(0x01, "(%s)db_open error[%s]\n", __func__,
			   db_strerror(ret));
		return ;
	}

	memset(&dbkey, 0, sizeof(dbkey));
	memset(&dbval, 0, sizeof(dbval));
	dbkey.data = tbl;
	dbkey.size = strlen(tbl);

	dbp->get(dbp, NULL, &dbkey, &dbval, 0);

	//size = dbval.size;
	json_reader_t* reader = json->create_json_reader(dbval.data);
	json_writer_t* writer = reader;
	str_ptr = json->get_json_string(reader, key, defval);
	if(strcmp(val, str_ptr) != 0) {
		json->set_json_string(writer, key, val);
		json->out_json_to_bytes(writer, &json_str[0]);

		memset(&dbval, 0, sizeof(dbval));
		dbval.data = &json_str[0];
		dbval.size = strlen(json_str);

		log->debug(0x01, "set_value(): %d, %s, %d, %s\n", strlen(tbl), tbl,
				strlen(json_str), json_str);

		dbp->put(dbp, NULL, &dbkey, &dbval, 0);
	}

	dbp->close(dbp, 0);
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
