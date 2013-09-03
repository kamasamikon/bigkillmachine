CFLAGS += -g -Wall 
CFLAGS += -I.

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: rlog 

rlog_SRCS = rlog.c klogwatch.c 
rlog: $(rlog_SRCS)
	gcc -o $@ $(rlog_SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm rlog 


