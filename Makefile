
export HI_PRJ_ROOT := $(CURDIR)

.PHONY: all clean

all: dagou daxia dr inotdo
clean: dagou.clean daxia.clean dr.clean inotdo.clean

-include Makefile.defs

#########################################################################
# Banner: show current configuration
#
.PHONY: banner
banner:
	@echo
	@echo "***************************************************************"
	@echo "* Architecture: $(BKM_PLATFORM)"
	@echo "* Debug: $(BKM_DEBUG)"
	@echo "***************************************************************"
	@echo

#########################################################################
# neccesary
#
.PHONY: amust
amust: banner

#########################################################################
# Architecture of target board
#
.PHONY: arch.x86
arch.x86: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.7231
arch.7231: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.sdtv
arch.sdtv: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

#########################################################################
# Architecture of target board
#
.PHONY: debug.yes
debug.yes: clean
	@sed -i "s/BKM_DEBUG := no/BKM_DEBUG := yes/g" $(HI_PRJ_ROOT)/.configure

.PHONY: debug.no
debug.no: clean
	@sed -i "s/BKM_DEBUG := yes/BKM_DEBUG := no/g" $(HI_PRJ_ROOT)/.configure

#########################################################################
# $(HI_PRJ_ROOT)/dagou
#
.PHONY: dagou
dagou: amust
	make -C $(HI_PRJ_ROOT)/dagou

.PHONY: dagou.clean
dagou.clean: amust
	make -C $(HI_PRJ_ROOT)/dagou clean

#########################################################################
# $(HI_PRJ_ROOT)/daxia
#
.PHONY: daxia
daxia: amust
	make -C $(HI_PRJ_ROOT)/daxia

.PHONY: daxia.clean
daxia.clean: amust
	make -C $(HI_PRJ_ROOT)/daxia clean

#########################################################################
# $(HI_PRJ_ROOT)/dr
#
.PHONY: dr
dr: amust
	make -C $(HI_PRJ_ROOT)/dr

.PHONY: dr.clean
dr.clean: amust
	make -C $(HI_PRJ_ROOT)/dr clean

#########################################################################
# $(HI_PRJ_ROOT)/inotdo
#
.PHONY: inotdo
inotdo: amust
	make -C $(HI_PRJ_ROOT)/inotdo

.PHONY: inotdo.clean
inotdo.clean: amust
	make -C $(HI_PRJ_ROOT)/inotdo clean

#########################################################################
# Target::install
#
.PHONY: install install.test
install: dagou
	@./build/install/${BKM_PLATFORM}
