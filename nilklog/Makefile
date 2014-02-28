export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_nilklog = \
				 $(HI_PRJ_ROOT)/nilklog/nilklog.o

LOCAL_OUT_OBJS = $(SUB_OBJS_nilklog)
LOCAL_OUT_LIB = nilklog.a
LOCAL_SHARE_LIB = libnilklog.so

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
