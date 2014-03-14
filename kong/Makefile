export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_kong = \
				 $(HI_PRJ_ROOT)/kong/kong.o

LOCAL_OUT_OBJS = $(SUB_OBJS_kong)
LOCAL_OUT_LIB = kong.a
LOCAL_SHARE_LIB = libkong.so

LDFLAGS := $(filter-out -lhilda,$(LDFLAGS))

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
