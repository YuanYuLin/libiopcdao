
TARGET = libiopcdao.so

CFLAGS += -I$(SDKSTAGE)/usr/include/libbdb
CFLAGS += -I$(SDKSTAGE)/usr/include/libtokyocabinet
CFLAGS += -I$(SDKSTAGE)/usr/include/libiopcmisc
CFLAGS += -I$(SDKSTAGE)/usr/include/
CFLAGS += -I./include/
CFLAGS += 

LDFLAGS += -L$(SDKSTAGE)/lib/ 
LDFLAGS += -L$(SDKSTAGE)/usr/lib/ 
LDFLAGS += -lc -lpthread -ldl
LDFLAGS += -ldb 

SRC = 

SRC += ops_db.c
SRC += ops_dummydb.c
SRC += ops_bdb.c
SRC += ops_tcdb.c
SRC += ops_ttdb.c

include Makefile.include.lib
