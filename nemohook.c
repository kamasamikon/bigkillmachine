/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/* XXX: define before include dlfcn.h */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <dlfcn.h>

/* LD_PRELOAD=/home/auv/nemohook.so ./prog1 */
/*-----------------------------------------------------------------------
 * helper
 */
int klogf(const char *fmt, ...);
int klogf(const char *fmt, ...)
{
	static FILE *fp = NULL;
	if (!fp)
		fp = fopen("/tmp/nemohook.out", "wt");

	va_list ap;
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	fflush(fp);
	va_end(ap);

	return 0;
}

/*-----------------------------------------------------------------------
 * sqlite
 */
#include <sqlite3.h>

static void sqliteTrace(void *arg, const char *query)
{
	klogf("sqliteTrace: <%s>\n", query);
}

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open");

	int ret = realfunc(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);
	klogf("NEMOHOOK: sqlite3_open: file:\"%s\", ret:%d\n", filename, ret);

	return ret;
}
int sqlite3_open16(const void *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open16");

	int ret = realfunc(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);
	klogf("NEMOHOOK: sqlite3_open16: file:\"%s\", ret:%d\n", (char*)filename, ret);

	return ret;
}
int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
	static int (*realfunc)(const char*, sqlite3**, int, const char*) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open_v2");

	int ret = realfunc(filename, ppDb, flags, zVfs);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);
	klogf("NEMOHOOK: sqlite3_open_v2: file:\"%s\", ret:%d\n", filename, ret);

	return ret;
}

/*-----------------------------------------------------------------------
 * dbus
 */
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-print-message.h"
dbus_bool_t bus_dispatch_matches (/* BusTransaction */ void *transaction,
		DBusConnection *sender,
		DBusConnection *addressed_recipient,
		DBusMessage *message,
		DBusError *error);

dbus_bool_t bus_dispatch_matches (/* BusTransaction */ void *transaction,
		DBusConnection *sender,
		DBusConnection *addressed_recipient,
		DBusMessage *message,
		DBusError *error)
{
	static dbus_bool_t (*realfunc)(void*, void*, void*, void*, void*) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "bus_dispatch_matches");

	print_message(message, FALSE);
	dbus_bool_t ret = realfunc(transaction, sender, addressed_recipient, message, error);
	printf("NEMOHOOK: bus_dispatch_matches: ret:%d\n", ret);

	return ret;
}

/*-----------------------------------------------------------------------
 * gconf
 */
#if 0
#include <gconf/gconf-client.h>

/* Return buf, if ret != buf, should call free(ret) */
static char *entry_value(const GConfValue *val, char *buf, int len)
{
	const char *tmp;

	switch (val->type) {
	case GCONF_VALUE_STRING:
		tmp = gconf_value_get_string(val);
		if (!tmp)
			return NULL;
		if (strlen(tmp) >= len)
			return strdup(tmp);
		strcpy(buf, tmp);
		return buf;

	case GCONF_VALUE_INT:
		sprintf(buf, "%d", gconf_value_get_int(val));
		return buf;

	case GCONF_VALUE_FLOAT:
		sprintf(buf, "%f", gconf_value_get_float(val));
		return buf;

	case GCONF_VALUE_BOOL:
		sprintf(buf, "%s", gconf_value_get_bool(val) ? "True" : "False");
		return buf;

	case GCONF_VALUE_SCHEMA:
	case GCONF_VALUE_LIST:
	case GCONF_VALUE_PAIR:
	default:
		/* TODO: do it */
		sprintf(buf, "??? Not supported !!!");
		return buf;
	}
}

GConfValue *gconf_sources_query_value(/* GConfSources */ void *sources,
		const gchar *key,
		const gchar **locales,
		gboolean use_schema_default,
		gboolean *value_is_default,
		gboolean *value_is_writable,
		gchar **schema_name,
		GError **err)
{
	char buf[256], *val;

	static GConfValue *(*realfunc)(void*, const gchar*, const gchar**, gboolean, gboolean*, gboolean*, gchar**, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_sources_query_value");

	GConfValue *ret = realfunc(sources, key, locales, use_schema_default, value_is_default, value_is_writable, schema_name, err);

	val = entry_value(ret, buf, sizeof(buf));
	klogf("NEMOHOOK: gconf_sources_query_value: key: <%s>, val:<%s>\n", key, val);
	if (val != buf)
		free(val);

	return ret;
}

void gconf_sources_set_value(/* GConfSources */ void *sources,
		const gchar *key,
		const GConfValue *value,
		/* GConfSources */ void **modified_sources,
		GError **err)
{
	char buf[256], *val;

	static void (*realfunc)(void*, const gchar*, const GConfValue*, void**, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_sources_set_value");

	realfunc(sources, key, value, modified_sources, err);

	val = entry_value(value, buf, sizeof(buf));
	klogf("NEMOHOOK: gconf_sources_set_value: key: <%s>, val:<%s>\n", key, val);
	if (val != buf)
		free(val);
}

void gconf_sources_unset_value(/* GConfSources */ void *sources,
		const gchar *key,
		const gchar *locale,
		/* GConfSources */ void **modified_sources,
		GError **err)
{
	static void (*realfunc)(void*, const gchar*, const gchar*, void**, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_sources_unset_value");

	realfunc(sources, key, locale, modified_sources, err);
	klogf("NEMOHOOK: gconf_sources_unset_value: key:<%s>\n", key);
}
#endif

/*-----------------------------------------------------------------------
 * ioctl - PI
 */
#include <sys/ioctl.h>

int ioctl(int d, unsigned long int request, ...)
{
	static int (*realfunc)(int d, unsigned long int request, void*) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "ioctl");

	va_list args;
	void *argp;

	va_start(args, request);
	argp = va_arg(args, void *);
	va_end(args);

	int ret = realfunc(d, request, argp);
	klogf("NEMOHOOK: ioctl: d:%d, request:%lu\n", d, request);

	return ret;
}

/*-----------------------------------------------------------------------
 * pcd
 */
/* XXX: Add -v to pcd command line */
