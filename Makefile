export HI_PRJ_ROOT := $(CURDIR)
export HI_PLATFORM := 'CURRENT'
export HI_BUILD := 'test'

#########################################################################
## /media/auv/Windows8_OS/Users/nemo/work/bigkillmachine/Makefile.defs
#
-include $(HI_PRJ_ROOT)/.configure

ifeq ($(HILDA_DEBUG),yes)
	CFLAGS += -g3
else
	CFLAGS += -O3
endif

CFLAGS += $(DEFINES) $(INCDIR)
LDFLAGS += $(LIBS)

MAKE += CC="$(CC)" AR="$(AR)" LD="$(LD)"

PATH := $(GCC_PATH):$(PATH)

.PHONY: all clean install uninstall
all: netmon
#########################################################################
## netmon
#
all: netmon.all
netmon.all:
	@echo make -C /media/auv/Windows8_OS/Users/nemo/work/bigkillmachine/SRCS/netmon all

clean: netmon.clean
netmon.clean:
	@echo make -C /media/auv/Windows8_OS/Users/nemo/work/bigkillmachine/SRCS/netmon clean

install: netmon.install
netmon.install:
	@echo make -C /media/auv/Windows8_OS/Users/nemo/work/bigkillmachine/SRCS/netmon install

uninstall: netmon.uninstall
netmon.uninstall:
	@echo make -C /media/auv/Windows8_OS/Users/nemo/work/bigkillmachine/SRCS/netmon uninstall

