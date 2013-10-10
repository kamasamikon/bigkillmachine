#-----------------------------------------------------------------
# Function define for nemotv only
#-----------------------------------------------------------------

define NEMOTV_PACKAGE_INNER
#  Arg 1: lower case package name
#  Arg 2: upper case package name

$(2)_SOURCE =

define  $(2)_EXTRACT_CMDS
cp  -rfp  $$($(2)_SITE)/*  $$(@D)
endef

$(2)_AUTORECONF = YES

ifeq  ($(COVERITY_DEMO),1)
define  $(2)_BUILD_CMDS
$(Q)echo  "Here  do  the  coverity  stuff..."
$(TARGET_MAKE_ENV)  $$($$(PKG)_MAKE_ENV)  $$($$(PKG)_MAKE)  \
$$($$(PKG)_MAKE_OPT)  -C  $$($$(PKG)_SRCDIR)
$(Q)echo  "Some  more  coverity  things..."
endef
else
define  $(2)_BUILD_CMDS
$(TARGET_MAKE_ENV)  $$($$(PKG)_MAKE_ENV)  $$($$(PKG)_MAKE)  \
$$($$(PKG)_MAKE_OPT)  -C  $$($$(PKG)_SRCDIR)
endef
endif  #  COVERITY_DEMO

#  For some  reason, all NemoTV packages are installed in /usr/local. It
#  would probably makes sense to install them in /usr, just like all
#  other packages.
$(2)_CONF_OPT  +=  --prefix=/usr/local  --exec-prefix=/usr/local

$(call  AUTOTARGETS,package/nemotv,$(1))

NTV_SRC  +=  $(1)
#  This is for backward compatibility only. Packages should be changed
#  to depend on nemotv-<pkg> instead.
$(subst  nemotv-,,$(1)):  $(1)
endef

define  NEMOTV_PACKAGE
$(call  NEMOTV_PACKAGE_INNER,$(1),$(call  UPPERCASE,$(1)))
endef


# Autoconf and Automake This is for the build-2010.11
NEMOTV_INSTALL_DIR = /usr/local
NTV_AUTORECONF_ENV:= AUTOCONF=$(BASE_DIR)/host/usr/bin/autoconf AUTOHEADER=$(BASE_DIR)/host/usr/bin/autoheader AUTOM4TE=$(BASE_DIR)/host/usr/bin/autom4te AUTOMAKE=$(BASE_DIR)/host/usr/bin/automake ACLOCAL=$(BASE_DIR)/host/usr/bin/aclocal LIBTOOLIZE=$(BASE_DIR)/host/usr/bin/libtoolize AUTOPOINT=$(BASE_DIR)/host/usr/bin/autopoint 

define DO_AUTOCONF_AUTOMAKE
	echo "`tput smso`>>> `basename $$(pwd)` is building... `tput rmso`"; \
	rm -rf config.cache; \
	for i in `find $(STAGING_DIR)/lib/ -name "*.la"`; do \
		$(SED) "s:\(['= ]\)/lib:\\1$(STAGING_DIR)/lib:g" $$i; \
	done; \
	for i in `find $(STAGING_DIR)/usr/lib/ -name "*.la"`; do \
		$(SED) "s:\(['= ]\)/usr/lib:\\1$(STAGING_DIR)/usr/lib:g" $$i; \
	done; \
	for i in `find $(STAGING_DIR)/usr/local/lib/ -name "*.la"`; do \
		$(SED) "s:\(['= ]\)/usr/local/lib:\\1$(STAGING_DIR)/usr/local/lib:g" $$i; \
	done; \
	$(BASE_DIR)/host/usr/bin/aclocal; \
	$(BASE_DIR)/host/usr/bin/automake --add-missing; \
	touch AUTHORS ChangeLog NEWS README; \
	$(NTV_AUTORECONF_ENV) $(BASE_DIR)/host/usr/bin/autoreconf --force --install --make; \
	for i in `find $${current_src_dir} -name ltmain.sh`; do \
		ltmain_version=`sed -n '/^[ 	]*VERSION=/{s/^[ 	]*VERSION=//;p;q;}' $$i | sed 's/\([0-9].[0-9]*\).*/\1/'`; \
		if test $${ltmain_version} = "1.5"; then \
			$(BUILDROOT_DIR)/support/scripts/apply-patches.sh $${i%/*} $(BUILDROOT_DIR)/support/libtool buildroot-libtool-v1.5.patch; \
		elif test $${ltmain_version} = "2.2"; then\
			$(BUILDROOT_DIR)/support/scripts/apply-patches.sh $${i%/*} $(BUILDROOT_DIR)/support/libtool buildroot-libtool-v2.2.patch; \
		elif test $${ltmain_version} = "2.4"; then\
			$(BUILDROOT_DIR)/support/scripts/apply-patches.sh $${i%/*} $(BUILDROOT_DIR)/support/libtool buildroot-libtool-v2.4.patch; \
		fi \
	done; \
	$(TARGET_CONFIGURE_OPTS) $(TARGET_CONFIGURE_ARGS) ./configure $(O_DEBUG_CFLAGS) --target=$(GNU_TARGET_NAME) \
	--host=$(GNU_TARGET_NAME) --build=$(GNU_HOST_NAME) --prefix=$(NEMOTV_INSTALL_DIR) --sysconfdir=/etc
endef
#--------------------------------------------------------------------------
# Variables defination for nemotv only.
#--------------------------------------------------------------------------
# This PLATFORM_TYPE (emulator|st7108|bcm7346|...) specific cflag is used temporarly for developer to enclose some
# C code for special platform. The value of it is the start with "TARGET_" and followed by the upcase of the
# platform folder name under "ntvtgt".
PLATFORM_TYPE=TARGET_$(shell echo $(notdir $(PWD)|tr a-z A-Z))

BRCM_TARGETS:= TARGET_BCM7346_UCLIBC_BC TARGET_TC7356_UCLIBC_BC TARGET_SAM7231_UCLIBC_BC TARGET_SAM7425_UCLIBC_BC TARGET_BCM7429_UCLIBC_BC

ST_TARGETS:= TARGET_ST7108_GLIBC_CT TARGET_ST239_GLIBC_CT

# The PLATFORM_VENDOR is used for platform-specific woraround in MW (NTV5_1-606).
PLATFORM_VENDOR:=PLATFORM_EMULATOR

ifneq ($(findstring $(PLATFORM_TYPE),$(BRCM_TARGETS)), )
PLATFORM_VENDOR=PLATFORM_BRCM
endif

ifneq ($(findstring $(PLATFORM_TYPE),$(ST_TARGETS)), )
PLATFORM_VENDOR=PLATFORM_ST
endif

ifeq ($(BUILD_TYPE),)
BUILD_TYPE:=debug
endif

ifeq ($(BUILD_TYPE),release)
# Setup CFLAGS for release build
O_BUILD_CFLAGS:=-DNDEBUG
	
ifeq ($(PLATFORM_VENDOR), PLATFORM_BRCM)

ifeq ($(CORE_DUMP),1)
# If CORE_DUMP is 1, turn on DEBUG_INIT=1 by default. 
DEBUG_INIT=1
O_BUILD_CFLAGS += -rdynamic -DCORE_DUMP
endif

ifeq ($(DEBUG_INIT),1)

ifeq ($(PERFHOOKS),1)
O_BUILD_CFLAGS += -DPERFHOOKS
endif

endif

endif

# Turn on Backtrace by default
ifeq ($(DEBUG_INIT),1)
O_BUILD_CFLAGS += -rdynamic -DBACKTRACE

endif

else # if BUILD_TYPE is "debug" or "bld"

# For BCM platform, backtrace could not work. So we enable core dump in default.
ifeq ($(PLATFORM_VENDOR),PLATFORM_BRCM)
CORE_DUMP:=1
endif

#set the CFLAGS for bld and debug build
# Eventually these are the options we need to impose
# O_BUILD_ERR = -Werror=shadow -Werror=undef -Werror=uninitialized -Werror=implicit -Werror=missing-prototypes -Werror=cast-align
# O_BUILD_WARN = -Wall -Wunreachable-code -Wparentheses -Wswitch
O_BUILD_CFLAGS:= -rdynamic -g -Wall -Wshadow  -Werror=uninitialized -Wunreachable-code -Wimplicit -Wparentheses -Wswitch -Wmissing-prototypes -Wcast-align  
ifneq ($(PLATFORM_TYPE),TARGET_ST7108_GLIBC_CT)
O_BUILD_CFLAGS += -Werror=undef
endif
ifeq ($(BUILD_TYPE),bld)
ifeq ($(DEBUG_ASSERT),1)
O_BUILD_CFLAGS += -DBACKTRACE -DNTVLOG_NO_STDOUT
else
O_BUILD_CFLAGS += -DNDEBUG -DBACKTRACE
endif
endif
ifeq ($(PERFHOOKS),1)
O_BUILD_CFLAGS += -DPERFHOOKS
endif
#
# TSS symbol to trace memory in framework
#
ifeq ($(OOC_MTRACE),1)
O_BUILD_CFLAGS += -DOOC_MTRACE
endif
ifeq ($(MTRACE),1)
O_BUILD_CFLAGS += -DMTRACE
endif
ifeq ($(TSS_DEBUG),1)
O_BUILD_CFLAGS += -DTSS_DEBUG
endif

ifeq ($(CORE_DUMP),1)
O_BUILD_CFLAGS += -DCORE_DUMP
endif

ifeq ($(EITS_METRICS),1)
O_BUILD_CFLAGS += -DEITS_METRICS
endif

endif # ifeq ($(BUILD_TYPE),release) 

ifeq ($(CONFIG_TYPE),pvr)
O_BUILD_CFLAGS+=-DNTV_PVR
endif


ifeq ($(CONFIG_TYPE),tf_e5)
O_BUILD_CFLAGS+=-DNTV_TF_E5
endif

ifneq ($(LOG_LEVEL), )
O_BUILD_CFLAGS+=-DLOG_LEVEL=$(LOG_LEVEL)
endif

O_BUILD_CFLAGS+=-D$(PLATFORM_TYPE) -D$(PLATFORM_VENDOR)

###
## ######################################################################
# Nemo: Append hilda stuff, Will overwrite O_DEBUG_CFLAGS
O_BUILD_CFLAGS += -lhilda

O_DEBUG_CFLAGS:="CFLAGS= $(TARGET_CFLAGS) $(O_BUILD_CFLAGS)"

export OUTPUT_DIR = $(BASE_DIR)

# the directory of NETWORK configration data 
NETCFG_DIR:=$(NEMOTV_DIR)/../netcfg

O_DEBUG_CFLAGS:="CFLAGS= $(TARGET_CFLAGS) $(O_BUILD_CFLAGS)"
#-------------------------------------------------------------------
# Make targets defination for nemotv only
#-------------------------------------------------------------------


#Only for emulator.Run emulator now.
#set the emulator log output fd(stdio/file)
ifneq ($(LOG), )
export CONSOLE=console=ttyS0
ifeq ($(LOG),stdio)
export LOG_OUTPUT:=$(LOG)
else
export LOG_OUTPUT:=file:$(LOG)
endif
endif

ifeq ($(BUILD_TYPE), release)
ifeq ($(BR2_PACKAGE_NEMOTV_SYSINIT_EMULATOR),y)
export BOOT_IP:=ip=dhcp
endif
endif
	
ifeq ($(DEBUG_INIT),1)
SHOW_LOG:=1
endif

ifeq ($(SHOW_LOG),)
SHOW_LOG:=0
endif


