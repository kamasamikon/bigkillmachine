/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/* XXX: define before include dlfcn.h */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <dlfcn.h>
#include <time.h>

#define NH_SQLITE

/* LD_PRELOAD=/home/auv/nemohook.so ./prog1 */
/*-----------------------------------------------------------------------
 * helper
 */
int klogf(const char *fmt, ...);
int klogf(const char *fmt, ...)
{
	char buffer[4096], *bufptr = buffer;
	static FILE *fp = NULL;
	int ret, ofs, bufsize = sizeof(buffer);

	va_list ap;

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
	if (!fp)
		fp = fopen("/tmp/nemohook.out", "wt");

	va_start(ap, fmt);
	ofs = 0;
	ofs += sprintf(bufptr + ofs, "%d|", (int)getpid());
	ofs += sprintf(bufptr + ofs, "%s|", tmbuf);
	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);

	if (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		if (bufptr != buffer)
			free(bufptr);
		bufptr = (char*)malloc(bufsize);

		ofs = 0;
		ofs += sprintf(bufptr + ofs, "%d|", (int)getpid());
		ofs += sprintf(bufptr + ofs, "%s|", tmbuf);
		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}
	va_end(ap);

	ret += ofs;
	va_start(ap, fmt);
	fwrite(bufptr, 1, ret, fp);
	fflush(fp);
	va_end(ap);
	if (bufptr != buffer)
		free(bufptr);

	return 0;
}

/*-----------------------------------------------------------------------
 * sqlite
 */
#ifdef NH_SQLITE
#include <sqlite3.h>

static void sqliteTrace(void *arg, const char *query)
{
	klogf("SQL: <%s>\n", query);
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
#endif

/*-----------------------------------------------------------------------
 * dbus
 */
#ifdef NH_DBUS
/*
 * XXX:
 * bus_dispatch_matches: Only used in dbus-daemon
 *
 * XXX:
 * Should add a patch to dbus/dbus-connection.c:dbus_connection_dispatch()
 * line 4558.
 *
 * message = message_link->data;
 * dbus_message_hook(message);
 */
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-print-message.h"

DBusDispatchStatus dbus_connection_dispatch(DBusConnection*connection);
DBusDispatchStatus dbus_connection_dispatch(DBusConnection*connection)
{
	static DBusDispatchStatus (*realfunc)(void**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "dbus_connection_dispatch");

	DBusDispatchStatus ret = realfunc(connection);
	klogf("NEMOHOOK: dbus_connection_dispatch: ret:%d\n", ret);

	return ret;
}
#endif

/*-----------------------------------------------------------------------
 * gconf
 */
#ifdef NH_GCONF
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
		GError **err);

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
		GError **err);
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
		GError **err);
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
#ifdef NH_IOCTL
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
	klogf("NEMOHOOK: ioctl: d:%d, request:%08lx\n", d, request);

	return ret;


	argp = para;
	request = REQUEST_of_IOW;

	klogf("ioctl: d:%d, req:%08x, dir:%08x, type:%08x, nr:%08x, size:%08x, argp:%08x\n",
			d, request, _IOC_DIR(request), _IOC_TYPE(), _IOC_NR(), _IOC_SIZE(), argp);

	/* used to decode ioctl numbers.. */
#define _IOC_DIR(nr)		(((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)		(((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)		(((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)		(((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)


}
#endif

/*-----------------------------------------------------------------------
 * pcd
 */
/* XXX: Add -v to pcd command line */

/*-----------------------------------------------------------------------
 * test
 */
#ifdef NH_TEST
int main()
{
	klogf("shit\n");
}
#endif
