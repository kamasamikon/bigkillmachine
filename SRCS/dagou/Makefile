export HI_PRJ_ROOT = ../..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_dagou = \
				 ./nbuf.o \
				 ./narg.o \
				 ./dbus-print-message.o \
				 ./helper.o \
				 ./dagou_dbus.o \
				 ./dagou_ioctol.o \
				 ./dagou_nsulog.o \
				 ./dagou_sqlite.o \
				 ./dagou_syslog.o \
				 ./dalog_setup.o \
				 ./dalog.o

				 # ./dagou_gconf.o \

LOCAL_OUT_OBJS = $(SUB_OBJS_dagou)
LOCAL_OUT_LIB = dagou.a
LOCAL_SHARE_LIB = libdagou.so

CFLAGS += -I.

CFLAGS += -I$(SYSROOT_DIR)/usr/include
CFLAGS += -I$(SYSROOT_DIR)/usr/include/glib-2.0
CFLAGS += -I$(SYSROOT_DIR)/usr/include/dbus-1.0
CFLAGS += -I$(SYSROOT_DIR)/usr/local/include
CFLAGS += -I$(SYSROOT_DIR)/usr/lib/glib-2.0/include
CFLAGS += -I$(SYSROOT_DIR)/usr/lib/dbus-1.0/include

LDFLAGS += -L$(SYSROOT_DIR)/lib
LDFLAGS += -L$(SYSROOT_DIR)/usr/lib

LDFLAGS += -lgconf-2 -lglib-2.0  
LDFLAGS += -ldbus-glib-1 -ldbus-1 -lpthread -lrt -lgobject-2.0 -lglib-2.0  
LDFLAGS += -ldbus-1 -lpthread -lrt  
LDFLAGS += -lsqlite3  
LDFLAGS += -lglib-2.0

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
