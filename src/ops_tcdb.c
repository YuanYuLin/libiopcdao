#include "ops_log.h"
#include "ops_misc.h"
#include "ops_dbutil.h"
#include "ops_db.h"
#include "ops_tcdb.h"

static void init(void)
{
}

static uint8_t get_boolean(uint8_t * key)
{
	uint8_t val = 0;
	return val;
}

static void set_boolean(uint8_t * key, uint8_t val)
{
}

static uint32_t get_uint32(uint8_t * key)
{
	uint32_t val = 0;
	return val;
}

static void set_uint32(uint8_t * key, uint32_t val)
{
}

static uint32_t get_string(uint8_t * key, uint8_t * val)
{
	uint32_t size = 0;
	return size;
}

static void set_string(uint8_t * key, uint8_t * val)
{
}

static struct ops_db_t *obj = NULL;
struct ops_db_t *get_tcdb_instance()
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

void del_tcdb_instance()
{
	if (obj)
		free(obj);
}
