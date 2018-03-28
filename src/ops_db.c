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
#include "ops_json.h"
#include "ops_db.h"

#define DAO_KV_MAX      128

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
};

struct dao_kv_512_t {
        struct dao_hdr_t hdr;
        uint8_t data[DAOTYPE_DATALEN_500];
};

static struct dao_kv_512_t dao_kv_list[DAO_KV_MAX];

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
			log->debug(0x01, "record used: %s\n", key);
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

static void init(void)
{
	int i = 0;
	int idx = 0;
	struct dao_kv_512_t* kv = NULL;
	for(i=0;i<DAO_KV_MAX;i++){
		memset(&dao_kv_list[i], 0, sizeof(struct dao_kv_512_t));
	}

	{
#define KEY_CONFIG_VERSION "config_version"
#define VAL_CONFIG_VERSION "{\"major\":0, \"minor\":0, \"aux\":0}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_CONFIG_VERSION);
	kv->hdr.val_len = strlen(VAL_CONFIG_VERSION);
	strcpy(&kv->data[0], KEY_CONFIG_VERSION);
	strcpy(&kv->data[kv->hdr.key_len], VAL_CONFIG_VERSION);
	}
#if 0
	{
#define KEY_CONFIGS "configs"
#define VAL_CONFIGS "[\"netifc_count\", \"storage_count\"]"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_CONFIGS);
	kv->hdr.val_len = strlen(VAL_CONFIGS);
	strcpy(&kv->data[0], KEY_CONFIGS);
	strcpy(&kv->data[kv->hdr.key_len], VAL_CONFIGS);
	}
#endif
	{
#define KEY_NETIFC_COUNT "netifc_count"
#define VAL_NETIFC_COUNT "[\"netifc_1\", \"netifc_2\",  \"netifc_3\", \"netifc_4\"]"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_NETIFC_COUNT);
	kv->hdr.val_len = strlen(VAL_NETIFC_COUNT);
	strcpy(&kv->data[0], KEY_NETIFC_COUNT);
	strcpy(&kv->data[kv->hdr.key_len], VAL_NETIFC_COUNT);
	}
	{
#define KEY_NETIFC_1 "netifc_1"
#define VAL_NETIFC_1 "{\"visable\":1, \"editable\":0, \"type\":\"eth\", \"name\":\"eth0\", \"vlan\":0, \"tag\":0, \"devices\":[], \"src\":\"none\"}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_NETIFC_1);
	kv->hdr.val_len = strlen(VAL_NETIFC_1);
	strcpy(&kv->data[0], KEY_NETIFC_1);
	strcpy(&kv->data[kv->hdr.key_len], VAL_NETIFC_1);
	}
	{
#define KEY_NETIFC_2 "netifc_2"
#define VAL_NETIFC_2 "{\"visable\":1, \"editable\":0, \"type\":\"eth\", \"name\":\"eth0\", \"vlan\":1, \"tag\":100, \"devices\":[], \"src\":\"none\"}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_NETIFC_2);
	kv->hdr.val_len = strlen(VAL_NETIFC_2);
	strcpy(&kv->data[0], KEY_NETIFC_2);
	strcpy(&kv->data[kv->hdr.key_len], VAL_NETIFC_2);
	}
	{
#define KEY_NETIFC_3 "netifc_3"
#define VAL_NETIFC_3 "{\"visable\":1, \"editable\":1, \"type\":\"bridge\", \"name\":\"br0\", \"vlan\":0, \"tag\":0, \"devices\":[\"eth0\"], \"src\":\"dhcp\"}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_NETIFC_3);
	kv->hdr.val_len = strlen(VAL_NETIFC_3);
	strcpy(&kv->data[0], KEY_NETIFC_3);
	strcpy(&kv->data[kv->hdr.key_len], VAL_NETIFC_3);
	}
	{
#define KEY_NETIFC_4 "netifc_4"
#define VAL_NETIFC_4 "{\"visable\":1, \"editable\":1, \"type\":\"bridge\", \"name\":\"br1\", \"vlan\":0, \"tag\":0, \"devices\":[\"eth0.100\"], \"src\":\"storage\", \"net_cfg_json\":\"/hdd/sys/extra_cfg.json\", \"ctrlport\":1}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_NETIFC_4);
	kv->hdr.val_len = strlen(VAL_NETIFC_4);
	strcpy(&kv->data[0], KEY_NETIFC_4);
	strcpy(&kv->data[kv->hdr.key_len], VAL_NETIFC_4);
	}
	{
#define KEY_STORAGE_COUNT "storage_count"
#define VAL_STORAGE_COUNT "[\"storage_1\"]"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_STORAGE_COUNT);
	kv->hdr.val_len = strlen(VAL_STORAGE_COUNT);
	strcpy(&kv->data[0], KEY_STORAGE_COUNT);
	strcpy(&kv->data[kv->hdr.key_len], VAL_STORAGE_COUNT);
	}
	{
#define KEY_STORAGE_1 "storage_1"
#define VAL_STORAGE_1 "{\"device\":\"/dev/sda\", \"start\":0, \"end\":262143, \"sector_size\":512, \"media_type\":\"hdd\", \"partitions\":[\"sda_part_1\", \"sda_part_2\", \"sda_part_3\", \"sda_part_4\"]}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_STORAGE_1);
	kv->hdr.val_len = strlen(VAL_STORAGE_1);
	strcpy(&kv->data[0], KEY_STORAGE_1);
	strcpy(&kv->data[kv->hdr.key_len], VAL_STORAGE_1);
	}
	{
#define KEY_SDA_PART_1 "sda_part_1"
#define VAL_SDA_PART_1 "{\"visable\":1, \"formatable\":0, \"type\":\"fat\", \"src\":\"/dev/sda1\", \"dst\":\"/hdd/sys\", \"label\":\"system\", \"start\":2048, \"sectors\":102400}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_SDA_PART_1);
	kv->hdr.val_len = strlen(VAL_SDA_PART_1);
	strcpy(&kv->data[0], KEY_SDA_PART_1);
	strcpy(&kv->data[kv->hdr.key_len], VAL_SDA_PART_1);
	}
	{
#define KEY_SDA_PART_2 "sda_part_2"
#define VAL_SDA_PART_2 "{\"visable\":0, \"formatable\":0, \"type\":\"sysdao\", \"src\":\"\", \"dst\":\"\", \"label\":\"sysdao1\", \"start\":104448, \"sectors\":10240}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_SDA_PART_2);
	kv->hdr.val_len = strlen(VAL_SDA_PART_2);
	strcpy(&kv->data[0], KEY_SDA_PART_2);
	strcpy(&kv->data[kv->hdr.key_len], VAL_SDA_PART_2);
	}
	{
#define KEY_SDA_PART_3 "sda_part_3"
#define VAL_SDA_PART_3 "{\"visable\":0, \"formatable\":0, \"type\":\"sysdao\", \"src\":\"\", \"dst\":\"\", \"label\":\"sysdao2\", \"start\":114688, \"sectors\":10240}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_SDA_PART_3);
	kv->hdr.val_len = strlen(VAL_SDA_PART_3);
	strcpy(&kv->data[0], KEY_SDA_PART_3);
	strcpy(&kv->data[kv->hdr.key_len], VAL_SDA_PART_3);
	}
	{
#define KEY_SDA_PART_4 "sda_part_4"
#define VAL_SDA_PART_4 "{\"visable\":1, \"formatable\":0, \"type\":\"fat\", \"src\":\"/dev/sda2\", \"dst\":\"/hdd/data\", \"label\":\"data\", \"start\":124928, \"sectors\":262143}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_SDA_PART_4);
	kv->hdr.val_len = strlen(VAL_SDA_PART_4);
	strcpy(&kv->data[0], KEY_SDA_PART_4);
	strcpy(&kv->data[kv->hdr.key_len], VAL_SDA_PART_4);
	}
	{
#define KEY_HOSTNAME "hostname_cfg"
#define VAL_HOSTNAME "{\"src\":\"storage\", \"hostname_cfg_json\":\"/hdd/sys/extra_cfg.json\"}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_HOSTNAME);
	kv->hdr.val_len = strlen(VAL_HOSTNAME);
	strcpy(&kv->data[0], KEY_HOSTNAME);
	strcpy(&kv->data[kv->hdr.key_len], VAL_HOSTNAME);
	}
	{
#define KEY_DRBD "drbd_cfg"
/*
#define VAL_DRBD "{\"enabled\":0, \"ipaddress_local\":\"\", \"macaddress_local\":\"\", \"hostname_local\":\"qemu1\", \"drbd_local\":\"/dev/drbd0\", \"disk_local\":\"/dev/sdb\", \"hostname_remote\":\"qemu2\", \"drbd_remote\":\"/dev/drbd0\", \"disk_remote\":\"/dev/sdb\", \"ipaddress_remote\":\"\", \"macaddress_remote\":\"\"}"
*/
#define VAL_DRBD "{\"ipaddress_local\":\"\", \"macaddress_local\":\"\", \"hostname_local\":\"qemu1\", \"drbd_local\":\"/dev/drbd0\", \"disk_local\":\"/dev/sdb\", \"hostname_remote\":\"qemu2\", \"drbd_remote\":\"/dev/drbd0\", \"disk_remote\":\"/dev/sdb\", \"ipaddress_remote\":\"\", \"macaddress_remote\":\"\"}"
	kv = &dao_kv_list[idx++];
	kv->hdr.chksum = 0;
	kv->hdr.type |= DAOTYPE_JSON_512;
	kv->hdr.resv = 0;
	kv->hdr.key_len = strlen(KEY_DRBD);
	kv->hdr.val_len = strlen(VAL_DRBD);
	strcpy(&kv->data[0], KEY_DRBD);
	strcpy(&kv->data[kv->hdr.key_len], VAL_DRBD);
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
