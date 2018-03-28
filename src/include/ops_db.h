#ifndef OPS_DB_H
#define OPS_DB_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

//#include "ops_mq.h"

//#define SOCKET_PATH_DB	"/var/run/uds.db"
//#define MAX_CLIENT_DB	5

struct ops_db_t {
	void (*init) (void);
	void (*show_all) (void);
	void (*del) (void);
//	 uint8_t(*get_boolean) (uint8_t* tbl, uint8_t * key);
//	void (*set_boolean) (uint8_t* tbl, uint8_t * key, uint8_t val);
//	 uint32_t(*get_uint32) (uint8_t* tbl, uint8_t * key);
//	void (*set_uint32) (uint8_t* tbl, uint8_t * key, uint32_t val);
//	 uint32_t(*get_string) (uint8_t* tbl, uint8_t * key, uint8_t * val);
//	void (*set_string) (uint8_t* tbl, uint8_t * key, uint8_t * val);

//	int (*uds_server_create)();
//	int (*uds_server_send)(int socket_fd, struct msg_t *msg, struct sockaddr_un* cli_addr, socklen_t cli_addr_len);
//	int (*uds_server_recv)(int socket_fd, struct msg_t *msg, struct sockaddr_un* cli_addr, socklen_t* cli_addr_len);
//	int (*uds_client_send_and_recv)(struct msg_t* req, struct msg_t* res);
//	void (*uds_close)(int socket_fd);

//	uint8_t (*process)(struct msg_t* req, struct msg_t* res);
	uint16_t (*get_val)(uint8_t* key, uint8_t* val);
	uint16_t (*set_val)(uint8_t* key, uint8_t* val);
};

#define KV_KEY		"#dbk#"
#define KV_VAL		"#dbv#"

#define MAX_DATA_SIZE		4096
#define DBVALLEN		1024

//#define DB_DATATYPE_BOOLEAN		"bool"
//#define DB_DATATYPE_UINT32		"ui32"
//#define DB_DATATYPE_STRING		"str"

//#define DB_TYPE_DUMMY		0
//#define DB_TYPE_BDB		1
//#define DB_TYPE_TCDB		2
//#define DB_TYPE_TTDB		3

struct ops_db_t *get_db_instance();
void del_db_instance();
#endif
