
export HI_PRJ_ROOT := $(CURDIR)

.PHONY: all clean

all: hook sewer
clean: hook.clean sewer.clean

-include Makefile.defs

#########################################################################
# Banner: show current configuration
#
.PHONY: banner
banner:
	@echo
	@echo "***************************************************************"
	@echo "* Architecture: $(HILDA_PLATFORM)"
	@echo "* Debug: $(HILDA_DEBUG)"
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

#########################################################################
# Architecture of target board
#
.PHONY: debug.yes
debug.yes: clean
	@sed -i "s/export HILDA_DEBUG=no/export HILDA_DEBUG=yes/g" $(HI_PRJ_ROOT)/.configure

.PHONY: debug.no
debug.no: clean
	@sed -i "s/export HILDA_DEBUG=yes/export HILDA_DEBUG=no/g" $(HI_PRJ_ROOT)/.configure

#########################################################################
# $(HI_PRJ_ROOT)/hook
#
.PHONY: hook
hook: amust
	make -C $(HI_PRJ_ROOT)/build/platform/linux

.PHONY: hook.clean
hook.clean: amust
	make -C $(HI_PRJ_ROOT)/build/platform/linux clean

#########################################################################
# $(HI_PRJ_ROOT)/klog-sewer
#
.PHONY: sewer
sewer: amust
	make -C $(HI_PRJ_ROOT)/klog-sewer

.PHONY: sewer.clean
sewer.clean: amust
	make -C $(HI_PRJ_ROOT)/klog-sewer clean


#########################################################################
# Target::install
#
.PHONY: install install.test
install: hook
	@./build/install/${HILDA_PLATFORM}
