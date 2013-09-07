CFLAGS += -g -Wall 

CFLAGS += `pkg-config --cflags --libs glib-2.0`
CFLAGS += `pkg-config --cflags --libs gtk+-2.0`
CFLAGS += `pkg-config --cflags --libs gconf-2.0`
CFLAGS += `pkg-config --cflags --libs dbus-glib-1`
CFLAGS += `pkg-config --cflags --libs dbus-1`
CFLAGS += `pkg-config --cflags --libs sqlite3`

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

ALL += klog sqlite

all: $(ALL)

klog_SRCS = test-klog.c 
klog: $(klog_SRCS)
	gcc -o $@ $(klog_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS) -DMODU_NAME=\"NHT_KLOG\"

sqlite_SRCS = test-sqlite.c 
sqlite: $(sqlite_SRCS)
	gcc -o $@ $(sqlite_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS) -DMODU_NAME=\"NHT_SQL\"

clean:
	rm $(ALL)

