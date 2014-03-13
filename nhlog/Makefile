export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_nhlog = $(HI_PRJ_ROOT)/nhlog/nhlog.o

LOCAL_OUT_OBJS = $(SUB_OBJS_nhlog)
LOCAL_OUT_LIB = nhlog.a
LOCAL_SHARE_LIB = libnhlog.so

CFLAGS += -I.
CFLAGS += -I$(SYSROOT_DIR)/usr/include

CFLAGS += -save-temps -Wl,-Map=nhlog.map

LDFLAGS += -L$(SYSROOT_DIR)/usr/lib

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules

