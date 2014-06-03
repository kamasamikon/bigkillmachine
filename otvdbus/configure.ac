# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# This file is part of Appman.
AC_PREREQ(2.63)

m4_define([libotvdbus_major_version], [0])
m4_define([libotvdbus_minor_version], [0])

m4_define([libotvdbus_micro_version], [1])

m4_define([libotvdbus_version],
          [libotvdbus_major_version.libotvdbus_minor_version.libotvdbus_micro_version])

AC_INIT([libotvdbus],[libotvdbus_version])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([otvdbus-controller.c])
AC_PROG_CC
AM_PROG_CC_C_O
LT_INIT

AC_HEADER_STDC

AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

DBUS_REQUIRED=1.2.16
DBUS_GLIB_REQUIRED=0.80
GLIB_REQUIRED=2.22.5
SYSUTILS_REQUIRED=0.1

# Check requirements for libtracker-client
NTVDBUS_LIB_REQUIRED="glib-2.0     >= $GLIB_REQUIRED
                            gobject-2.0  >= $GLIB_REQUIRED
                            gio-unix-2.0 >= $GLIB_REQUIRED
                            dbus-1       >= $DBUS_REQUIRED
                            dbus-glib-1  >= $DBUS_GLIB_REQUIRED"

SYSUTIL_LIB_REQUIRED="utils  >= $SYSUTILS_REQUIRED"

PKG_CHECK_MODULES(NTVDBUS_LIB, [$NTVDBUS_LIB_REQUIRED])

PKG_CHECK_MODULES(SYSUTILS, [$SYSUTIL_LIB_REQUIRED])


BUILD_CFLAGS="-DG_LOG_DOMAIN=\\\"NTVDBUS\\\" -DNTVDBUS_COMPILATION -DO_LOG_LEVEL=O_WARNING"


GLIB_GENMARSHAL=`$PKG_CONFIG glib-2.0 --variable=glib_genmarshal`
AC_SUBST(GLIB_GENMARSHAL)


## TODO ADD REQUIREMENT FOR AIM PACKAGE

AC_SUBST(BUILD_CFLAGS)

##AC_DISABLE_STATIC

AC_CONFIG_FILES([
Makefile
tests/Makefile
otvdbus.pc
])

AC_OUTPUT
