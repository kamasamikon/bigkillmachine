CFLAGS += -g -Wall 

HILDA_FLAGS += -I `echo $$HILDA_INC`
HILDA_FLAGS += -L `echo $$HILDA_LIB`
HILDA_FLAGS += -lhilda

all: dsq

SRCS = bootpar.c

dsq: $(SRCS)
	gcc -o $@ $(SRCS) $(CFLAGS) $(LDFLAGS) $(HILDA_FLAGS)

clean:
	rm dsq



