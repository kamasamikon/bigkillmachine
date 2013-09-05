CFLAGS += -g -Wall 

CFLAGS += `pkg-config --cflags --libs glib-2.0`
CFLAGS += `pkg-config --cflags --libs gtk+-2.0`
CFLAGS += `pkg-config --cflags --libs gconf-2.0`
CFLAGS += `pkg-config --cflags --libs dbus-glib-1`
CFLAGS += `pkg-config --cflags --libs dbus-1`

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: xlog 

xlog_SRCS = xlog.c
xlog: $(xlog_SRCS)
	gcc -o $@ $(xlog_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm xlog 

