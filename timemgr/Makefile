export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_hook = \
				$(HI_PRJ_ROOT)/timemgr/timemgr.o \
				$(HI_PRJ_ROOT)/timemgr/timemgr_controller.o \
				$(HI_PRJ_ROOT)/timemgr/timemgr_utils.o \
				$(HI_PRJ_ROOT)/timemgr/timemgr_updater.o \
				$(HI_PRJ_ROOT)/timemgr/timeproxy.o 

LOCAL_OUT_OBJS = $(SUB_OBJS_hook)
LOCAL_OUT_ELF = timemgr

CFLAGS += -save-temps
CFLAGS += -I.
CFLAGS += -I$(HI_PRJ_ROOT)/otvdbus
CFLAGS += -I$(SYSROOT_DIR)/usr/include
CFLAGS += -I$(SYSROOT_DIR)/usr/include/glib-2.0
CFLAGS += -I$(SYSROOT_DIR)/usr/include/dbus-1.0
CFLAGS += -I$(SYSROOT_DIR)/usr/local/include
CFLAGS += -I$(SYSROOT_DIR)/usr/lib/glib-2.0/include
CFLAGS += -I$(SYSROOT_DIR)/usr/lib/dbus-1.0/include

LDFLAGS += -L$(SYSROOT_DIR)/usr/lib
LDFLAGS += -Wl,-rpath=.

LDFLAGS += -lgconf-2 -lglib-2.0  
LDFLAGS += -ldbus-glib-1 -ldbus-1 -lpthread -lrt -lgobject-2.0 -lglib-2.0  
LDFLAGS += -ldbus-1 -lpthread -lrt  
LDFLAGS += -lsqlite3  
LDFLAGS += -lglib-2.0
LDFLAGS += -L$(HI_PRJ_ROOT)/otvdbus -lotvdbus

.PHONY: all clean

all: $(LOCAL_OUT_ELF) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
