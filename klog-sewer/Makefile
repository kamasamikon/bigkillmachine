CFLAGS += -g -Wall 

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: klsewer

klogsewer_SRCS = klog-sewer.c
klsewer: $(klogsewer_SRCS)
	gcc -o $@ $(klogsewer_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm klsewer

