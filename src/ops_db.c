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
#include <stdlib.h>

#include "zlib.h"
#include "ops_misc.h"
#include "ops_log.h"
#include "ops_json.h"
#include "ops_db.h"

//#define DAO_KV_MAX      128

/*
 * bit 0 ~ 3 : DAOTYPE
 * bit 4 :
 * bit 5 :
 * bit 6 :
 * bit 7 : last record
 */
#define DAOTYPE_UNKNOWN         0x00
#define DAOTYPE_JSON_512        0x01

#define DAOTYPE_LAST_RECORD     0x80
#define DAOTYPE_DATALEN_500	500

struct dao_hdr_t {
        uint32_t chksum;
        uint8_t type;
        uint8_t key_len;
        uint16_t val_len;
        uint32_t resv;
} __attribute__((packed));

struct dao_kv_512_t {
        struct dao_hdr_t hdr;
        uint8_t data[DAOTYPE_DATALEN_500];
};

#include "db_init.inc"

static uint16_t get_dao_val(uint8_t* key, uint8_t* val)
{
	struct ops_log_t* log = get_log_instance();
	int i = 0;
	uint16_t val_size = 0;
	struct dao_kv_512_t* kv = NULL;
	for(i=0;i<DAO_KV_MAX;i++) {
		kv = &dao_kv_list[i];
		if((kv->hdr.type & 0x0F) == DAOTYPE_UNKNOWN) {
			log->debug(0x01, "key: unknown\n");
			break;
		} else if((kv->hdr.type & 0x0F) == DAOTYPE_JSON_512) {
			//log->debug(0x01, "record used: %s\n", key);
			if((strlen(key) == kv->hdr.key_len) && (memcmp(key, &kv->data[0], kv->hdr.key_len) == 0)) {
				memcpy(val, &kv->data[kv->hdr.key_len], kv->hdr.val_len);
				val_size = kv->hdr.val_len;
				log->debug(0x01, "record copied %s, %s\n", key, val);
				break;
			}
		} else {
			log->debug(0x01, "key: not found\n");
		}
	}
	log->debug(0x01, "key: %s, val[%d]: %s\n", key, val_size, val);
	return val_size;
}

static uint16_t set_dao_val(uint8_t* key, uint8_t* val)
{
	struct ops_log_t* log = get_log_instance();
	uint16_t val_size = 0;
	int i = 0;
	struct dao_kv_512_t* kv = NULL;
	log->debug(0x1, "%s-key: %s\n", __func__, key);
	log->debug(0x1, "%s-val: %s\n", __func__, val);
	for(i=0;i<DAO_KV_MAX;i++) {
		kv = &dao_kv_list[i];
		if((kv->hdr.type & 0x0F) == DAOTYPE_JSON_512) {
			if((strlen(key) == kv->hdr.key_len) && (memcmp(key, &kv->data[0], kv->hdr.key_len) == 0)) {
				// key found, set new value
				memset(&kv->data[kv->hdr.key_len], 0, kv->hdr.val_len);
				kv->hdr.val_len = strlen(val);
				memcpy(&kv->data[kv->hdr.key_len], val, kv->hdr.val_len);
				val_size = kv->hdr.val_len;
				break;
			}
			continue;
		} else if((kv->hdr.type & 0x0F) == DAOTYPE_UNKNOWN) {
			kv->hdr.type |= DAOTYPE_JSON_512;
			kv->hdr.key_len = strlen(key);
			kv->hdr.val_len = strlen(val);
			memcpy(&kv->data[0], key, kv->hdr.key_len);
			memcpy(&kv->data[kv->hdr.key_len], val, kv->hdr.val_len);
			val_size = kv->hdr.val_len;
			break;
		}
	}
	return val_size;
}

struct part_t {
	uint8_t tag[12];
	uint8_t name[10];
	uint8_t fstype[10];
	int64_t start;
	int64_t end;
	uint8_t boot;
};

static void init(void)
{
	uint8_t buff[64];
	struct part_t* part;
	struct ops_log_t* log = get_log_instance();
	uint8_t* boot_dev = getenv("BOOT_DEV");
	int fd = -1;
	uint8_t rc = 0;
	uint64_t off_start = (1*1024*1024);
	uint64_t off_end = 0;
	uint8_t iopcfsdao_found = 0;
	int count = 0;
	//uint32_t crc32_1 = 0;
	//uint32_t crc32_2 = 0;
	fd = open(boot_dev, O_RDONLY);
	lseek(fd, off_start, SEEK_SET);
	log->debug(0x01, "%s - offset to %d\n", boot_dev, off_start);
	for(;;) {
		if(count > 1024) {
			break;
		}
#define IOPCHEAD	"$[IOPCHEAD]$"
#define IOPCEND		"$[IOPCEND]$"
#define IOPCREC		"$[IOPCREC]$"
#define IOPCFSDAO	"iopcdao"
		rc = read(fd, &buff, sizeof(buff));
		log->debug(0x01, "%d-read count %d\n", count, rc);

		if(memcmp(buff, IOPCHEAD, strlen(IOPCHEAD)) == 0) {
			log->debug(0x01, "IOPCHEAD\n");
		}
		if(memcmp(buff, IOPCEND, strlen(IOPCEND)) == 0) {
			log->debug(0x01, "IOPCEND\n");
			break;
		}
		if(memcmp(buff, IOPCREC, strlen(IOPCREC)) == 0) {
			log->debug(0x01, "IOPCREC\n");
			part = (struct part_t*)&buff;
			if(memcmp(part->fstype, IOPCFSDAO, strlen(IOPCFSDAO)) == 0) {
				off_start = part->start;
				off_end = part->end;
				iopcfsdao_found = 1;
				log->debug(0x01, "DAO start:%ld, end:%ld\n", off_start, off_end);
				break;
			}
		}
		count++;
	}
	close(fd);
	if(iopcfsdao_found) {
		fd = open(boot_dev, O_RDONLY);
		lseek(fd, off_start, SEEK_SET);
		rc = read(fd, &dao_kv_list, sizeof(dao_kv_list));
		/*
		crc32_1 = dao_kv.chksum;
		dao_kv.chksum = 0;
		crc32_2 = crc32(0, &dao_kv, sizeof(struct dao_kv_512_t));
		*/
		close(fd);
	}
}

static void show_all(void)
{
}

static struct ops_db_t *obj = NULL;
struct ops_db_t *get_db_instance()
{
	if (!obj) {
		obj = malloc(sizeof(struct ops_db_t));
		obj->init = init;
		obj->show_all = show_all;

		obj->get_val = get_dao_val;
		obj->set_val = set_dao_val;
	}
	return obj;
}

void del_db_instance()
{
	if (obj)
		free(obj);
}
