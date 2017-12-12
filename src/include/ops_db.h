#ifndef OPS_DB_H
#define OPS_DB_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

struct ops_db_t {
	void (*init) (void);
	 uint8_t(*get_boolean) (uint8_t * key);
	void (*set_boolean) (uint8_t * key, uint8_t val);
	 uint32_t(*get_uint32) (uint8_t * key);
	void (*set_uint32) (uint8_t * key, uint32_t val);
	 uint32_t(*get_string) (uint8_t * key, uint8_t * val);
	void (*set_string) (uint8_t * key, uint8_t * val);
};

#define DB_DATATYPE_BOOLEAN		"bool"
#define DB_DATATYPE_UINT32		"ui32"
#define DB_DATATYPE_STRING		"str"

#define DB_TYPE_DUMMY		0
#define DB_TYPE_BDB		1
#define DB_TYPE_TCDB		2
#define DB_TYPE_TTDB		3

struct ops_db_t *get_db_instance();
void del_db_instance();
#endif
