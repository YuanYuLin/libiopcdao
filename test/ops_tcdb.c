#include <tcutil.h>
#include <tcadb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "ops_log.h"
#include "ops_json.h"
#include "ops_misc.h"
#include "ops_dbutil.h"
#include "ops_db.h"
#include "ops_tcdb.h"

static TCADB *adb;

static void init(void)
{
	adb = tcadbnew();
	if(!tcadbopen(adb, DB_PATH_DEFAULT_TCDB)){
		fprintf(stderr, "open error\n");
	}
}

static void del(void)
{
	/* close the database */
	if(!tcadbclose(adb)){
		fprintf(stderr, "close error\n");
	}
	/* delete the object */
	tcadbdel(adb);
}

static uint8_t get_boolean(uint8_t* tbl, uint8_t * key)
{
	uint8_t val = 0;
	uint8_t defval = 0;
	struct ops_json_t* json = get_json_instance();
	uint8_t* json_str = tcadbget2(adb, tbl);
	json_reader_t* reader = json->create_json_reader(json_str);
	val = json->get_json_boolean(reader, key, defval);
	return val;
}

static void set_boolean(uint8_t* tbl, uint8_t * key, uint8_t val)
{
	uint8_t defval = (~val)?1:0;
	uint8_t data[MAX_DATA_SIZE] = {0};
	uint32_t data_len = 0;
	struct ops_json_t* json = get_json_instance();
	struct ops_log_t* log = get_log_instance();
	uint8_t* json_str = tcadbget2(adb, tbl);
	json_reader_t* reader = json->create_json_reader(json_str);
	json_writer_t* writer = reader;
	if(val != json->get_json_boolean(reader, key, defval)) {
		json->set_json_boolean(writer, key, val);
		data_len = json->out_json_to_bytes(writer, &data[0]);
		if(tcadbput2(adb, tbl, data)) {
			log->error(0x01, "error set_boolean - tbl:%s, key:%s, val:%d, dl:%ld\n", tbl, key, val, data_len);
		}
	}
}

static uint32_t get_uint32(uint8_t* tbl, uint8_t * key)
{
	uint32_t val = 0;
	uint32_t defval = 0;
	struct ops_json_t* json = get_json_instance();
	uint8_t* json_str = tcadbget2(adb, tbl);
	json_reader_t* reader = json->create_json_reader(json_str);
	val = json->get_json_int(reader, key, (int)defval);
	return val;
}

static void set_uint32(uint8_t* tbl, uint8_t * key, uint32_t val)
{
	uint8_t data[MAX_DATA_SIZE] = {0};
	uint32_t defval = val + 1;
	uint32_t data_len = 0;
	struct ops_json_t* json = get_json_instance();
	struct ops_log_t* log = get_log_instance();
	uint8_t* json_str = tcadbget2(adb, tbl);
	json_reader_t* reader = json->create_json_reader(json_str);
	json_writer_t* writer = reader;
	if(val != json->get_json_int(reader, key, (int)defval)) {
		json->set_json_int(writer, key, val);
		data_len = json->out_json_to_bytes(writer, &data[0]);
		if(tcadbput2(adb, tbl, data)) {
			log->error(0x01, "error set_boolean - tbl:%s, key:%s, val:%d, dl:%ld\n", tbl, key, val, data_len);
		}
	}
}

static uint32_t get_string(uint8_t* tbl, uint8_t * key, uint8_t * val)
{
	uint8_t* defval = NULL;
	uint32_t size = 0;
	struct ops_json_t* json = get_json_instance();
	uint8_t* json_str = tcadbget2(adb, tbl);
	json_reader_t* reader = json->create_json_reader(json_str);
	val = json->get_json_string(reader, key, defval);
	size = strlen(val);
	return size;
}

static void set_string(uint8_t* tbl, uint8_t * key, uint8_t * val)
{
	uint8_t data[MAX_DATA_SIZE] = {0};
	uint8_t* str_ptr = NULL;
	uint8_t* defval = NULL;
	uint32_t data_len = 0;
	struct ops_json_t* json = get_json_instance();
	struct ops_log_t* log = get_log_instance();
	uint8_t* json_str = tcadbget2(adb, tbl);
	json_reader_t* reader = json->create_json_reader(json_str);
	json_writer_t* writer = reader;
	str_ptr = json->get_json_string(reader, key, defval);
	if(strcmp(val, str_ptr) != 0) {
		json->set_json_string(writer, key, val);
		data_len = json->out_json_to_bytes(writer, &data[0]);
		if(tcadbput2(adb, tbl, data)) {
			log->error(0x01, "error set_boolean - tbl:%s, key:%s, val:%d, dl:%ld\n", tbl, key, val, data_len);
		}
	}
}

static struct ops_db_t *obj = NULL;
struct ops_db_t *get_tcdb_instance()
{
	if (!obj) {
		obj = malloc(sizeof(struct ops_db_t));
		obj->init = init;
		obj->del = del;
		obj->get_boolean = get_boolean;
		obj->set_boolean = set_boolean;
		obj->get_uint32 = get_uint32;
		obj->set_uint32 = set_uint32;
		obj->get_string = get_string;
		obj->set_string = set_string;
	}

	return obj;
}

void del_tcdb_instance()
{
	if (obj)
		free(obj);
}

