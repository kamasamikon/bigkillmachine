CFLAGS += -g -Wall

.PHONY: all

all: klsewer

klogsewer_SRCS = klog-sewer.c
klsewer: $(klogsewer_SRCS)
	gcc -o $@ $(klogsewer_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm klsewer -frv

