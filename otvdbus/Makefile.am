
SUBDIRS = . tests

NTVDBUS_PC=otvdbus.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(NTVDBUS_PC)

BUILD_ERR=-Werror=shadow -Werror=undef -Werror=uninitialized -Werror=implicit -Werror=missing-prototypes -Werror=cast-align
BUILD_WARN=-Wall -Wunreachable-code -Wparentheses -Wswitch -Werror


AM_CFLAGS = $(BUILD_WARN) $(BUILD_ERR)

AM_CPPFLAGS = \
	     $(BUILD_CFLAGS)     \
	      -I$(top_srcdir)    \
	      -I$(top_builddir)  \
	      $(NTVDBUS_LIB_CFLAGS) \
	      $(SYSUTILS_CFLAGS) 


lib_LTLIBRARIES = libotvdbus.la

libotvdbus_la_SOURCES = \
			$(marshal_sources)      \
			otvdbus-server.c	\
			otvdbus-connection.c	\
			otvdbus-controller.c	\
			otvdbus_util.c 
				

libotvdbus_la_HEADERS = otvdbus-controller.h	\
			otvdbus-server.h	\
			otvdbus-connection.h   \
			otvdbus_util.h

libotvdbus_ladir = $(includedir)/otvdbus

libotvdbus_la_LIBADD =  $(NTVDBUS_LIB_LIBS) $(SYSUTILS_LIBS)

marshal_sources =  \
	           otv-marshal.h \
		   otv-marshal.c

otv-marshal.h: otv-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --prefix=otv_marshal --header > $@

otv-marshal.c: otv-marshal.list
	$(AM_V_GEN)echo "#include \"otv-marshal.h\"" > $@ && \
		$(GLIB_GENMARSHAL) $< --prefix=otv_marshal --body >> $@


BUILT_SOURCES = \
		$(marshal_sources)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = otv-marshal.list \
	     otvdbus.pc.in
