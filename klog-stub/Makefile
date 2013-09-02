CFLAGS += -g -Wall 

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: klog klogserv

klog_SRCS = klogwatch.c
klog: $(klog_SRCS)
	gcc -o $@ $(klog_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS) -save-temps

klogserv_SRCS = klog-server.c
klogserv: $(klogserv_SRCS)
	gcc -o $@ $(klogserv_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm klog klogserv


