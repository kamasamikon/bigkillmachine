CFLAGS += -g -Wall 

CFLAGS += `pkg-config --cflags --libs glib-2.0`
CFLAGS += `pkg-config --cflags --libs gtk+-2.0`
CFLAGS += `pkg-config --cflags --libs gconf-2.0`
CFLAGS += `pkg-config --cflags --libs dbus-glib-1`
CFLAGS += `pkg-config --cflags --libs dbus-1`

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: bootpar nemohook

bootpar_SRCS = bootpar.c
bootpar: $(bootpar_SRCS)
	gcc -o $@ $(bootpar_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

nemohook_SRCS = nemohook.c dbus-print-message.c
nemohook: $(nemohook_SRCS)
	gcc -shared -ldl -fPIC $(CFLAGS) $(LDFLAGS) $(nemohook_SRCS) -o libnemohook.so

clean:
	rm bootpar klogserv klogclient

