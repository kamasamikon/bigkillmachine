export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_dagou = \
				 $(HI_PRJ_ROOT)/dagou/nbuf.o \
				 $(HI_PRJ_ROOT)/dagou/dbus-print-message.o \
				 $(HI_PRJ_ROOT)/dagou/dagou_dbus.o \
				 $(HI_PRJ_ROOT)/dagou/dagou_ioctol.o \
				 $(HI_PRJ_ROOT)/dagou/dagou_nsulog.o \
				 $(HI_PRJ_ROOT)/dagou/dagou_sqlite.o \
				 $(HI_PRJ_ROOT)/dagou/dagou_syslog.o \
				 $(HI_PRJ_ROOT)/dagou/dalog_setup.o \
				 $(HI_PRJ_ROOT)/dagou/dalog/dalog.o

				 # $(HI_PRJ_ROOT)/dagou/dagou_gconf.o \

LOCAL_OUT_OBJS = $(SUB_OBJS_dagou)
LOCAL_OUT_LIB = dagou.a
LOCAL_SHARE_LIB = libdagou.so

CFLAGS += -I$(HI_PRJ_ROOT)/dagou
CFLAGS += -I$(HI_PRJ_ROOT)/dagou/dalog

CFLAGS += -I$(SYSROOT_DIR)/usr/include
CFLAGS += -I$(SYSROOT_DIR)/usr/include/glib-2.0
CFLAGS += -I$(SYSROOT_DIR)/usr/include/dbus-1.0
CFLAGS += -I$(SYSROOT_DIR)/usr/local/include
CFLAGS += -I$(SYSROOT_DIR)/usr/lib/glib-2.0/include
CFLAGS += -I$(SYSROOT_DIR)/usr/lib/dbus-1.0/include

LDFLAGS += -L$(SYSROOT_DIR)/usr/lib

LDFLAGS += -lgconf-2 -lglib-2.0  
LDFLAGS += -ldbus-glib-1 -ldbus-1 -lpthread -lrt -lgobject-2.0 -lglib-2.0  
LDFLAGS += -ldbus-1 -lpthread -lrt  
LDFLAGS += -lsqlite3  
LDFLAGS += -lglib-2.0

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
