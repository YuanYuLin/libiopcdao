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
#include <dirent.h>

#include "zlib.h"
#include "ops_misc.h"
#include "ops_log.h"
#include "ops_json.h"
#include "ops_db.h"

#define MAX_BLOCK_LEN	128

#define DAOTYPE_MASK		0x7F
#define DAOTYPE_ENABLED		0x80
/*
 * bit 0 ~ 3 : DAOTYPE
 * bit 4 :
 * bit 5 :
 * bit 6 :
 * bit 7 : last record
 */
#define DAOTYPE_UNKNOWN         0x00
#define DAOTYPE_JSON            0x01

//#define DAOTYPE_LAST_RECORD     0x80
//#define DAOTYPE_DATALEN_500	500
/*
struct dao_hdr_t {
	uint8_t tag[8];//[*REC*]
        uint32_t chksum;
        uint8_t type;
        uint8_t key_len;
        uint16_t val_len;
	uint8_t rsv[8];//512-24=488
} __attribute__((packed));

struct dao_kv_t {
        struct dao_hdr_t hdr;
        uint8_t data[DAOTYPE_DATALEN];
};
*/

struct boot_info_t {
	uint8_t tag[8];//[*IOPC*]
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t rsv[54];
} __attribute__((packed));

struct dao_info_t {
	uint8_t  tag[8];//[*DAO*]
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t size_of_record;
	uint16_t count_of_records;
	uint16_t head_size_of_record;
	uint8_t rsv[48];
} __attribute__((packed));

#include "db_init.inc"

static uint16_t get_dao_val(uint8_t* key, uint8_t* val)
{
	struct ops_log_t* log = get_log_instance();
	int i = 0;
	uint16_t val_size = 0;
	struct dao_kv_t* kv = NULL;
	for(i=0;i<g_di.count_of_records;i++) {
		kv = &g_dao_kv_list[i];
		if(kv->hdr.type & DAOTYPE_ENABLED) {
			if((kv->hdr.type & DAOTYPE_MASK) == DAOTYPE_JSON) {
				//log->debug(0x01, "record used: %s\n", key);
				if((strlen(key) == kv->hdr.key_len) && (memcmp(key, &kv->data[0], kv->hdr.key_len) == 0)) {
					memcpy(val, &kv->data[kv->hdr.key_len], kv->hdr.val_len);
					val_size = kv->hdr.val_len;
					log->debug(0x01, __FILE__, __func__, __LINE__, "record[%d] copied %s, %s", i, key, val);
					break;
				}
			} else {
				log->error(0xFF, __FILE__, __func__, __LINE__, "key[%d]: not found", i);
			}
		}
	}
	log->debug(0x01, __FILE__, __func__, __LINE__, "key: %s, val[%d]: %s", key, val_size, val);
	return val_size;
}

static uint16_t set_dao_val(uint8_t* key, uint8_t* val)
{
	struct ops_log_t* log = get_log_instance();
	uint16_t val_size = 0;
	int i = 0;
	struct dao_kv_t* kv = NULL;
	log->debug(0x1, __FILE__, __func__, __LINE__, "key: %s, val: %s", key, val);
	int datalen = strlen(key) + strlen(val);
	if(datalen > g_di.size_of_record) {
		//not valid key/val record.
		return 0;
	}

	for(i=0;i<g_di.count_of_records;i++) {
		kv = &g_dao_kv_list[i];
		if(kv->hdr.type  == (DAOTYPE_ENABLED | DAOTYPE_JSON)) {
			if((strlen(key) == kv->hdr.key_len) && (memcmp(key, &kv->data[0], kv->hdr.key_len) == 0)) {
				// key found, set new value
				memset(&kv->data[kv->hdr.key_len], 0, kv->hdr.val_len);
				kv->hdr.val_len = strlen(val);
				memcpy(&kv->data[kv->hdr.key_len], val, kv->hdr.val_len);
				val_size = kv->hdr.val_len;
				break;
			}
			continue;
		} else if((kv->hdr.type & DAOTYPE_MASK) == DAOTYPE_UNKNOWN) {
			kv->hdr.type |= (DAOTYPE_ENABLED | DAOTYPE_JSON);
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

static int is_iopc_bootdev(uint8_t* dev_name)
{
    int fd = -1;
    uint32_t offset = BOOT_INFO_OFFSET;
    uint8_t magic[] = { '[', '*', 'I', 'O', 'P', 'C', '*', ']'};
    struct boot_info_t boot_info;
    fd = open(dev_name, O_RDONLY, 0644);
    if(fd < 0) {
        return 0;
    }
    lseek(fd, offset, SEEK_SET);
    read(fd, &boot_info, sizeof(struct boot_info_t));
    close(fd);
    if(memcmp(&boot_info.tag[0], &magic[0], sizeof(boot_info.tag)) == 0) {
        return 1;
    }
    return 0;
}

static int write_dao_info(uint8_t* dev_name)
{
    //struct ops_log_t* log = get_log_instance();
    int fd = -1;
    fd = open(dev_name, O_WRONLY, 0644);
    if(fd < 0) {
        return 0;
    }
    lseek(fd, DAO_INFO_OFFSET, SEEK_SET);
    write(fd, &g_di, sizeof(struct dao_info_t));
    close(fd);
    return 1;
}

static int reset_dao_info(uint8_t* dev_name)
{
    //struct ops_log_t* log = get_log_instance();
    int fd = -1;
    int info_size = sizeof(struct dao_info_t);
    uint8_t data[info_size];
    fd = open(dev_name, O_WRONLY, 0644);
    if(fd < 0) {
        return 0;
    }
    memset(&data[0], 0, info_size);
    lseek(fd, DAO_INFO_OFFSET, SEEK_SET);
    write(fd, &data, info_size);
    close(fd);
    return 1;
}

static int read_dao_info(uint8_t* dev_name, struct dao_info_t* info)
{
    int fd = -1;
    uint8_t magic[] = {'[', '*', 'D', 'A', 'O', '*', ']'};
    struct dao_info_t dao_info;
    fd = open(dev_name, O_RDONLY, 0644);
    if(fd < 0) {
        return 0;
    }
    lseek(fd, DAO_INFO_OFFSET, SEEK_SET);
    read(fd, &dao_info, sizeof(struct dao_info_t));
    close(fd);
    if(memcmp(&dao_info.tag[0], &magic[0], sizeof(magic)) == 0) {
        memcpy(info, &dao_info, sizeof(struct dao_info_t));
        return 1;
    }
    return 0;
}

static int write_dao_records(uint8_t* dev_name)
{
    uint16_t count_of_records = g_di.count_of_records;
    uint16_t size_of_record = g_di.size_of_record;
    int fd = -1;
    fd = open(dev_name, O_WRONLY, 0644);
    if(fd < 0) {
        return 0;
    }
    lseek(fd, DAO_DATA_OFFSET, SEEK_SET);
    write(fd, &g_dao_kv_list, (count_of_records * size_of_record));
    close(fd);
    return 1;
}

static int read_dao_records(uint8_t* dev_name, struct dao_info_t* dao_info)
{
    uint16_t count_of_records = dao_info->count_of_records;
    uint16_t size_of_record = dao_info->size_of_record;
    //uint16_t head_size_of_record = dao_info->head_size_of_record;
    struct dao_kv_t tmp_dao_list[count_of_records];
    struct ops_log_t* log = get_log_instance();
    int fd = -1;

    if(count_of_records <= 0)
        return 0;

    fd = open(dev_name, O_RDONLY, 0644);
    if(fd < 0) {
	return 0;
    }

    lseek(fd, DAO_DATA_OFFSET, SEEK_SET);
    read(fd, &tmp_dao_list, (size_of_record * count_of_records));
    close(fd);

    for(uint16_t idx=0;idx < count_of_records; idx++) {
        struct dao_kv_t *kv = &tmp_dao_list[idx];
	uint8_t *_key = NULL;
	uint8_t *_val = NULL;
	if(kv->hdr.type == (DAOTYPE_ENABLED | DAOTYPE_JSON)) {
	    _key = malloc(kv->hdr.key_len + 1);
	    _val = malloc(kv->hdr.val_len + 1);
	    memset(_key, 0, kv->hdr.key_len + 1);
	    memset(_val, 0, kv->hdr.val_len + 1);

	    memcpy(_key, &kv->data[0], kv->hdr.key_len);
	    memcpy(_val, &kv->data[kv->hdr.key_len], kv->hdr.val_len);
            log->debug(0xFF, __FILE__, __func__, __LINE__, "%d/%d, %x, %x, %x, %s, %s", idx, count_of_records, kv->hdr.type, kv->hdr.key_len, kv->hdr.val_len, _key, _val);
	    set_dao_val(_key, _val);

	    free(_key);
	    free(_val);
	}
    }

    return 0;
}

static void load_iopc_dao_from_storage()
{
    uint8_t boot_dev[MAX_BLOCK_LEN] = { 0 };
    struct dao_info_t dao_info;
    struct dirent *de;
    DIR* dir = opendir("/sys/block");
    if(dir == NULL) {
        return ;
    }
    memset(&dao_info, 0, sizeof(struct dao_info_t));
    while((de = readdir(dir)) != NULL) {
        memset(&boot_dev[0], 0, strlen(boot_dev));
        sprintf(boot_dev, "/dev/%s", de->d_name);
	if(is_iopc_bootdev(&boot_dev[0])) {
            if(read_dao_info(&boot_dev[0], &dao_info))
                read_dao_records(&boot_dev[0], &dao_info);
            break;
        }
    }
    closedir(dir);

    return ;
}

static void save_iopc_dao_to_storage()
{
    struct ops_log_t* log = get_log_instance();
    uint8_t boot_dev[MAX_BLOCK_LEN] = { 0 };
    struct dao_info_t dao_info;
    struct dirent *de;
    DIR* dir = opendir("/sys/block");
    if(dir == NULL) {
        return ;
    }
    memset(&dao_info, 0, sizeof(struct dao_info_t));
    while((de = readdir(dir)) != NULL) {
        memset(&boot_dev[0], 0, strlen(boot_dev));
        sprintf(boot_dev, "/dev/%s", de->d_name);
	if(is_iopc_bootdev(&boot_dev[0])) {
            write_dao_info(&boot_dev[0]);
            write_dao_records(&boot_dev[0]);
            break;
        }
        log->info(0x01, __FILE__, __func__, __LINE__, "bootdev %s", boot_dev);
    }
    closedir(dir);

    return ;
}

static void reset_iopc_dao_to_default()
{
    struct ops_log_t* log = get_log_instance();
    uint8_t boot_dev[MAX_BLOCK_LEN] = { 0 };
    struct dao_info_t dao_info;
    struct dirent *de;
    DIR* dir = opendir("/sys/block");
    if(dir == NULL) {
        return ;
    }
    memset(&dao_info, 0, sizeof(struct dao_info_t));
    while((de = readdir(dir)) != NULL) {
        memset(&boot_dev[0], 0, strlen(boot_dev));
        sprintf(boot_dev, "/dev/%s", de->d_name);
	if(is_iopc_bootdev(&boot_dev[0])) {
            reset_dao_info(&boot_dev[0]);
            break;
        }
        log->info(0x01, __FILE__, __func__, __LINE__, "bootdev %s", boot_dev);
    }
    closedir(dir);

    return ;
}

static void load_iopc_dao_from_binary()
{
	struct ops_log_t* log = get_log_instance();
	uint8_t* platform = getenv("PLATFORM");
	uint8_t dao_path[100] = {0};
	memset(dao_path, 0, sizeof(dao_path));
	if(platform) {
		sprintf(dao_path, "/lib/iopcdao/dao.%s.bin", platform);
	} else {
		sprintf(dao_path, "/lib/iopcdao/dao.bin");
	}
	int fd = open(dao_path, O_RDONLY);
	if(fd < 0) {
		log->error(0xFF, __FILE__, __func__, __LINE__, "DAO loaded error[%s]", dao_path);
		return;
	}
	read(fd, &g_dao_kv_list, sizeof(g_dao_kv_list));
	close(fd);
	// Print default data
	struct dao_kv_t* kv = NULL;
	for(int i=0;i<DAO_KV_MAX;i++) {
		kv = &g_dao_kv_list[i];
		log->debug(0x01, __FILE__, __func__, __LINE__, "%d, %x, %x, %x", i, kv->hdr.type, kv->hdr.key_len, kv->hdr.val_len);
		uint8_t brief_key[10]={0};
		if((kv->hdr.type & 0x0F) == DAOTYPE_JSON) {
			memcpy(&brief_key[0], &kv->data[0], (kv->hdr.key_len >= 9)?9:kv->hdr.key_len);
			log->debug(0x01, __FILE__, __func__, __LINE__, "record used: %s", brief_key);
		}
	}

}

static void init(void)
{
	load_iopc_dao_from_binary();
	load_iopc_dao_from_storage();
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

		obj->save_iopc_storage = save_iopc_dao_to_storage;
		obj->reset_iopc_storage = reset_iopc_dao_to_default;
	}
	return obj;
}

void del_db_instance()
{
	if (obj)
		free(obj);
}
