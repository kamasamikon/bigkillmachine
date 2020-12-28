export BKM_PRJ_ROOT = ../..
-include $(BKM_PRJ_ROOT)/Makefile.defs

LOCAL_OUT_ELF = inotdo
LOCAL_OUT_OBJS = inotdo.o 

LOCAL_CFLAGS += -O3

.PHONY: all clean

LOCAL_CFLAGS += -g -save-temps -Wl,-Map=inotdo.map,--cref

all: $(LOCAL_OUT_ELF) $(LOCAL_OUT_OBJS) 

-include $(BKM_PRJ_ROOT)/Makefile.rules
