CFLAGS += -g -Wall

CFLAGS += `pkg-config --cflags --libs glib-2.0`
CFLAGS += `pkg-config --cflags --libs gtk+-2.0`
CFLAGS += `pkg-config --cflags --libs gconf-2.0`
CFLAGS += `pkg-config --cflags --libs dbus-glib-1`
CFLAGS += `pkg-config --cflags --libs dbus-1`

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB/x86`
HILDA_FLAGS += -lhilda

all: libnemohook.so

nemohook_SRCS = dbus-print-message.c nh_dbus.c nh_gconf.c nh_helper.c nh_ioctl.c nh_sqlite.c nh_klog.c
libnemohook.so: $(nemohook_SRCS)
	gcc -shared -ldl -fPIC $(CFLAGS) $(LDFLAGS) $(nemohook_SRCS) $(HILDA_FLAGS) -o libnemohook.so

clean:
	rm libnemohook.so

