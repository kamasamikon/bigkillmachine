export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_nsulog = \
				 $(HI_PRJ_ROOT)/nsulog/nsulog.o

LOCAL_OUT_OBJS = $(SUB_OBJS_nsulog)
LOCAL_OUT_LIB = nsulog.a
LOCAL_SHARE_LIB = libnsulog.so

LDFLAGS := $(filter-out -lhilda,$(LDFLAGS))

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
