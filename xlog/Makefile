CFLAGS += -g -Wall 

CFLAGS += `pkg-config --cflags --libs glib-2.0`
CFLAGS += `pkg-config --cflags --libs gtk+-2.0`
CFLAGS += `pkg-config --cflags --libs gconf-2.0`
CFLAGS += `pkg-config --cflags --libs dbus-glib-1`
CFLAGS += `pkg-config --cflags --libs dbus-1`

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: klog 

klog_SRCS = klog.c
klog: $(klog_SRCS)
	gcc -o $@ $(klog_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS) -DMODU_NAME=\"XLOG_MODU\"

clean:
	rm klog 

