CFLAGS += -g -Wall 

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: bootpar klogserv klogclient

bootpar_SRCS = bootpar.c
bootpar: $(bootpar_SRCS)
	gcc -o $@ $(bootpar_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

klogserv_SRCS = klog-server.c
klogserv: $(klogserv_SRCS)
	gcc -o $@ $(klogserv_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

klogclient_SRCS = klog-client.c
klogclient: $(klogclient_SRCS)
	gcc -o $@ $(klogclient_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm bootpar



