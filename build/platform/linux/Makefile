export HI_PRJ_ROOT = ../../..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_hook = \
				 $(HI_PRJ_ROOT)/hook/nh_dbus.o \
				 $(HI_PRJ_ROOT)/hook/dbus-print-message.o \
				 $(HI_PRJ_ROOT)/hook/nh_gconf.o \
				 $(HI_PRJ_ROOT)/hook/nh_helper.o \
				 $(HI_PRJ_ROOT)/hook/nh_ioctl.o \
				 $(HI_PRJ_ROOT)/hook/nh_klog.o \
				 $(HI_PRJ_ROOT)/hook/nh_sqlite.o

LOCAL_INCDIRS = -I $(HI_PRJ_ROOT)/inc
LOCAL_OUT_OBJS = $(SUB_OBJS_hook)
LOCAL_OUT_LIB = nemohook.a
LOCAL_SHARE_LIB = libnemohook.so

.PHONY: all clean

all: $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
