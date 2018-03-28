#ifndef DAO_H
#define DAO_H

#include "ops_mq.h"

#define KV_KEY		"#dbk#"
#define KV_VAL		"#dbv#"

void init_dao();
uint8_t process_dao(struct msg_t* req, struct msg_t* res);

#endif
