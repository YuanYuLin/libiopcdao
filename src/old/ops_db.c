#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>

#include "ops_misc.h"
#include "ops_log.h"
#include "ops_db.h"
#include "dao.h"
//#include "ops_bdb.h"
//#include "ops_tcdb.h"
//#include "ops_ttdb.h"
//#include "ops_dummydb.h"
#if 0
#define CMD_NO_1	1
#define CMD_NO_2	2
#define CMD_FN_1	1
static int server_create()
{
	int socket_fd = -1;
	struct sockaddr_un addr;
	struct ops_log_t* log = get_log_instance();
	
	socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(socket_fd < 0) {
		log->error(0x01, "socket error : %s\n", strerror(errno));
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	log->debug(0x01, "serv create socket path: %s\n", SOCKET_PATH_DB);
	strncpy(addr.sun_path, SOCKET_PATH_DB, sizeof(addr.sun_path)-1);
	unlink(SOCKET_PATH_DB);
	if(bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		log->error(0x01, "bind error : %s\n", strerror(errno));
		return -2;
	}

	return socket_fd;
}

static int server_send(int socket_fd, struct msg_t *msg, struct sockaddr_un* cli_addr, socklen_t cli_addr_len)
{
	int wc = 0;
	uint16_t msg_size = sizeof(struct msg_t) - MAX_MSG_DATA_SIZE + msg->data_size;
	wc = sendto(socket_fd, (void*)msg, msg_size, 0, (struct sockaddr*)cli_addr, cli_addr_len);
	return wc;
}

static int server_recv(int socket_fd, struct msg_t *msg, struct sockaddr_un* cli_addr, socklen_t* cli_addr_len)
{
	int rc = 0;
	struct ops_log_t* log = get_log_instance();
	rc = recvfrom(socket_fd, msg, sizeof(struct msg_t), 0, (struct sockaddr*)cli_addr, cli_addr_len);
	if(rc < 0) {
	log->debug(0x01, "%s-%s-%d: %d, %x\n", __FILE__, __func__, __LINE__, rc, errno);
	switch(errno) {
	case EAGAIN:
		log->debug(0x01, "%s-%s-%d:EAGAIN\n", __FILE__, __func__, __LINE__);
	break;
//	case EWOULDBLOCK:
//		log->debug(0x01, "%s-%s-%d:EWOULDBLOCK\n", __FILE__, __func__, __LINE__);
//	break;
	case EINTR:
		log->debug(0x01, "%s-%s-%d:EINTR\n", __FILE__, __func__, __LINE__);
	break;
	case EBADF:
		log->debug(0x01, "%s-%s-%d:EBADF\n", __FILE__, __func__, __LINE__);
	break;
	case EINVAL:
		log->debug(0x01, "%s-%s-%d:EINVAL\n", __FILE__, __func__, __LINE__);
	break;
	case ECONNREFUSED:
		log->debug(0x01, "%s-%s-%d:ECONNREFUSED\n", __FILE__, __func__, __LINE__);
	break;
	case EFAULT:
		log->debug(0x01, "%s-%s-%d:EFAULT\n", __FILE__, __func__, __LINE__);
	break;
	case ENOMEM:
		log->debug(0x01, "%s-%s-%d:ENOMEM\n", __FILE__, __func__, __LINE__);
	break;
	case ENOTCONN:
		log->debug(0x01, "%s-%s-%d:ENOTCONN\n", __FILE__, __func__, __LINE__);
	break;
	case ENOTSOCK:
		log->debug(0x01, "%s-%s-%d:ENOTSOCK\n", __FILE__, __func__, __LINE__);
	break;
	default:
		log->debug(0x01, "%s-%s-%d:%d\n", __FILE__, __func__, __LINE__, errno);
	break;
	}
	}
	return rc;
}

static void close_socket(int socket_fd) 
{
	if(socket_fd < 0) {
	} else {
		close(socket_fd);
	}
}
static volatile uint16_t cli_cnt = 0;
static int client_send_and_recv(struct msg_t* req, struct msg_t* res)
{
	struct sockaddr_un addr;
	struct ops_log_t* log = get_log_instance();
	int wc = 0;
	int rc = 0;
//	uint32_t i = 0;
	int socket_fd = -1;
        struct sockaddr_un cli_addr;
        socklen_t cli_addr_len;

	socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (socket_fd == -1) {
		log->error(0x01, "cli socket: %s\n", strerror(errno));
		return -1;
	}

	memset(&cli_addr, 0, sizeof(struct sockaddr_un));
	cli_addr.sun_family = AF_UNIX;
	cli_cnt++;
	sprintf(cli_addr.sun_path, "%s.cli_%x", SOCKET_PATH_DB, cli_cnt);
#if 0
	for(i=0;i<MAX_CLIENT_DB;i++) {
		sprintf(cli_addr.sun_path, "%s.cli_%x", SOCKET_PATH_DB, i);
		if(access(cli_addr.sun_path, F_OK) != -1) {
			// file exist
			continue;
		} else {
			// file not exist
			break;
		}
	}
	if(i >= MAX_CLIENT_DB) {
		log->error(0x01, "can not find slot for clie\n");
	}
#endif
	log->debug(0x01, "bind path: %s\n", cli_addr.sun_path);

	if(bind(socket_fd, (struct sockaddr*)&cli_addr, sizeof(struct sockaddr_un)) < 0) {
		log->error(0x01, "cli bind error : %s\n", strerror(errno));
		return -2;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, SOCKET_PATH_DB);
	log->debug(0x01, "cli sending to %s\n", addr.sun_path);
	wc = server_send(socket_fd, req, &addr, sizeof(struct sockaddr_un));
	//wc = sendto(socket_fd, (void*)req, msg_size, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
	log->debug(0x01, "cli write count = %ld\n", wc);

	rc = server_recv(socket_fd, res, &cli_addr, &cli_addr_len);
	//rc = recvfrom(socket_fd, (void*)res, msg_size, 0, NULL, NULL);
	log->debug(0x01, "db cli read count = %ld\n", rc);

	close_socket(socket_fd);
	log->debug(0x01, "cli reading from %s\n", cli_addr.sun_path);
	unlink(cli_addr.sun_path);
	return 0;
}

static uint16_t get_dao_val(uint8_t* key, uint8_t* val)
{
	uint32_t msg_size = sizeof(struct msg_t);
	struct msg_t req_msg;
	struct msg_t res_msg;

	memset(&req_msg, 0, msg_size);
	memset(&res_msg, 0, msg_size);

	req_msg.fn = CMD_FN_1;
	req_msg.cmd = CMD_NO_1;
	sprintf(&req_msg.data[0], "{\"%s\":\"%s\"}", KV_KEY, key);
	req_msg.data_size = strlen(req_msg.data);

	client_send_and_recv(&req_msg, &res_msg);
	strcpy(val, res_msg.data);
	return res_msg.data_size;
}

static uint16_t set_dao_val(uint8_t* key, uint8_t* val)
{
	uint32_t msg_size = sizeof(struct msg_t);
	struct msg_t req_msg;
	struct msg_t res_msg;

	memset(&req_msg, 0, msg_size);
	memset(&res_msg, 0, msg_size);

	req_msg.fn = CMD_FN_1;
	req_msg.cmd = CMD_NO_2;
	sprintf(&req_msg.data[0], "{\"%s\":\"%s\", \"%s\":%s}", KV_KEY, key, KV_VAL, val);
	req_msg.data_size = strlen(req_msg.data);

	client_send_and_recv(&req_msg, &res_msg);
	strcpy(val, res_msg.data);
	return res_msg.data_size;
}
#endif
static void init(void)
{
	init_dao();
}

static void show_all(void)
{
}

static struct ops_db_t *obj = NULL;
struct ops_db_t *get_db_instance()
{
	//uint32_t db_type = DB_TYPE_DUMMY;
	//struct ops_db_t *dao = NULL;
	//struct ops_misc_t *misc = get_misc_instance();
	//struct ops_log_t *log = get_log_instance();
	if (!obj) {
		obj = malloc(sizeof(struct ops_db_t));
		obj->init = init;
		obj->show_all = show_all;
#if 0
		obj->process = process_dao;

		obj->uds_server_create = server_create;
		obj->uds_server_send = server_send;
		obj->uds_server_recv = server_recv;
		obj->uds_client_send_and_recv = client_send_and_recv;
		obj->uds_close = close_socket;
#endif
		obj->get_val = get_dao_val;
		obj->set_val = set_dao_val;
	}
/*
	if (misc->get_dbtype(&db_type) < 0) {
		db_type = DB_TYPE_DUMMY;
	}
	log->debug(0x01, "dbtype %x\n", db_type);
	switch (db_type) {
	case DB_TYPE_BDB:
		dao = get_bdb_instance();
		break;
	case DB_TYPE_TCDB:
		dao = get_tcdb_instance();
		break;
	case DB_TYPE_TTDB:
		//dao = get_ttdb_instance();
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
*/
	return obj;
}

void del_db_instance()
{
	if (obj)
		free(obj);
}
