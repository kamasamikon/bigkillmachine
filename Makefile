
export HI_PRJ_ROOT := $(CURDIR)

.PHONY: all clean

all: dagou sewer agent ldmon nhlog kong klbench
clean: dagou.clean sewer.clean agent.clean ldmon.clean nhlog.clean kong.clean klbench.clean

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
# $(HI_PRJ_ROOT)/klog-sewer
#
.PHONY: sewer
sewer: amust
	make -C $(HI_PRJ_ROOT)/klog-sewer

.PHONY: sewer.clean
sewer.clean: amust
	make -C $(HI_PRJ_ROOT)/klog-sewer clean

#########################################################################
# $(HI_PRJ_ROOT)/klog-agent
#
.PHONY: agent
agent: amust
	make -C $(HI_PRJ_ROOT)/klog-agent

.PHONY: agent.clean
agent.clean: amust
	make -C $(HI_PRJ_ROOT)/klog-agent clean

#########################################################################
# $(HI_PRJ_ROOT)/klog-bench
#
.PHONY: klbench
klbench: amust
	make -C $(HI_PRJ_ROOT)/klog-bench

.PHONY: klbench.clean
klbench.clean: amust
	make -C $(HI_PRJ_ROOT)/klog-bench clean

#########################################################################
# $(HI_PRJ_ROOT)/load-monitor
#
.PHONY: ldmon
ldmon: amust
	make -C $(HI_PRJ_ROOT)/load-monitor

.PHONY: ldmon.clean
ldmon.clean: amust
	make -C $(HI_PRJ_ROOT)/load-monitor clean

#########################################################################
# $(HI_PRJ_ROOT)/nhlog
#
.PHONY: nhlog
nhlog: amust
	make -C $(HI_PRJ_ROOT)/nhlog

.PHONY: nhlog.clean
nhlog.clean: amust
	make -C $(HI_PRJ_ROOT)/nhlog clean

#########################################################################
# $(HI_PRJ_ROOT)/kong
#
.PHONY: kong
kong: amust
	make -C $(HI_PRJ_ROOT)/kong

.PHONY: kong.clean
kong.clean: amust
	make -C $(HI_PRJ_ROOT)/kong clean

#########################################################################
# Target::install
#
.PHONY: install install.test
install: dagou
	@./build/install/${BKM_PLATFORM}
