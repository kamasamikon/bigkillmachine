CFLAGS += -g -Wall 

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: bootpar nemohook

bootpar_SRCS = bootpar.c
bootpar: $(bootpar_SRCS)
	gcc -o $@ $(bootpar_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

nemohook:
	gcc -shared -ldl -fPIC nemohook.c -o libnemohook.so

clean:
	rm bootpar klogserv klogclient

