# DO NOT BUILD TESTS BY DEFAULT 
# SUBDIRS = . tests

SYSUTILS_PC=utils.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(SYSUTILS_PC)

SYSUTILS_CFLAGS = -D_GNU_SOURCE

AM_CPPFLAGS = 				\
	      $(SYSUTILS_LIB_CFLAGS)	\
	      $(SYSUTILS_CFLAGS)        \
	      $(GLIB_DEPS_CFLAGS)

AM_CFLAGS = 				\
	      $(SYSUTILS_LIB_CFLAGS)	\
	      $(SYSUTILS_CFLAGS)

lib_LTLIBRARIES = libutils.la

#============================================================================================================================
# COMPILER ERROR OPTIONS. 
# DO NOT REMOVE
# DO NOT CONVERT BACK TO WARNINGS
#
# Build root already imposes the following in CFLAGS:
#   MINIMUM WARNINGS: -Wall -Wunreachable-code -Wparentheses -Wswitch
#   MINUMUM ERRORS  : -Werror=uninitialized -Werror=undef
#
# Need to add AT LEAST:
#   REQUIRED ERRORS : -Werror=shadow -Werror=implicit -Werror=missing-prototypes -Werror=cast-align  
#============================================================================================================================
BUILD_ERR = -Werror=shadow -Werror=implicit -Werror=missing-prototypes -Werror=cast-align 
BUILD_WARN = 
AM_CFLAGS += $(BUILD_ERR) $(BUILD_WARN)

libutils_la_SOURCES =	\
	oshm.c      		\
	opmutex.c   		\
	otimer.c    		\
	dalog.c dalog_setup.c narg.c nsulog.c nbuf.c \
	otrace.c                \
	utilsdefs.c		\
	softdog.c		\
	opthread.c  		\
	odbgtrap.c		\
	userpool.c              \
	osignal.c           

##
# NOTE:
# Only expose the files that are public. utilsdefs.h are PRIVATE.
# otracepriv is also private, but the traceparser needs it. 
libutils_la_HEADERS =$(top_srcdir)/otypes.h \
	$(top_srcdir)/outils.h \
	$(top_srcdir)/osll.h \
	$(top_srcdir)/otimer.h \
	$(top_srcdir)/dalog.h $(top_srcdir)/dalog_setup.h $(top_srcdir)/narg.h $(top_srcdir)/nsulog.h $(top_srcdir)/nbuf.h $(top_srcdir)/helper.h \
	$(top_srcdir)/ntvlog.h \
	$(top_srcdir)/userpool.h \
	$(top_srcdir)/osignal.h \
	$(top_srcdir)/otrace.h \
	$(top_srcdir)/otracepriv.h

libutils_la_LIBADD =  $(SYSUTILS_LIBS) -lrt

libutils_ladir = $(includedir)/utils


