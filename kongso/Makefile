export HI_PRJ_ROOT = ..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_kongso = \
				 $(HI_PRJ_ROOT)/kongso/kongso.o

LOCAL_OUT_OBJS = $(SUB_OBJS_kongso)
LOCAL_OUT_LIB = kongso.a
LOCAL_SHARE_LIB = libkongso.so

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
