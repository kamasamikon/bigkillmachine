export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_hook = \
				 $(HI_PRJ_ROOT)/hook/log_monitor.o \
				 $(HI_PRJ_ROOT)/hook/nh_dbus.o \
				 $(HI_PRJ_ROOT)/hook/dbus-print-message.o \
				 $(HI_PRJ_ROOT)/hook/nh_helper.o \
				 $(HI_PRJ_ROOT)/hook/nh_ioctl.o \
				 $(HI_PRJ_ROOT)/hook/nh_syslog.o \
				 $(HI_PRJ_ROOT)/hook/nh_klog.o \
				 $(HI_PRJ_ROOT)/hook/nh_sqlite.o

LOCAL_OUT_OBJS = $(SUB_OBJS_hook)
LOCAL_OUT_LIB = nemohook.a
LOCAL_SHARE_LIB = libnemohook.so

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
