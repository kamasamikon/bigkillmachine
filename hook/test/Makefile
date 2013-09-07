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

ALL += test

.PHONY: all
all: $(ALL)

test_SRCS = test-main.c test-klog.c test-sqlite.c
test: $(test_SRCS)
	gcc -o $@ $(test_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS) -DMODU_NAME=\"NHT_test\"

clean:
	rm $(ALL)

