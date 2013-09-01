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

#define NH_IOCTL
#define NH_GCONF
#define NH_DBUS
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

static int __g_sqlite_klog = 0;

static void sqliteTrace(void *arg, const char *query)
{
	if (__g_sqlite_klog)
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

	if (__g_sqlite_klog)
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
	if (__g_sqlite_klog)
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
	if (__g_sqlite_klog)
		klogf("NEMOHOOK: sqlite3_open_v2: file:\"%s\", ret:%d\n", filename, ret);

	return ret;
}
#endif

/*-----------------------------------------------------------------------
 * dbus
 */
#ifdef NH_DBUS

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-print-message.h"

static int __g_dbus_klog = 0;

void dbus_connection_dispatch_hook(DBusMessage *message);
void dbus_connection_dispatch_hook(DBusMessage *message)
{
	static void (*realfunc)(DBusMessage *message) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "dbus_connection_dispatch_hook");

	realfunc(message);
	if (__g_dbus_klog)
		klogf("NEMOHOOK: dbus_connection_dispatch_hook\n");
}
#endif

/*-----------------------------------------------------------------------
 * gconf
 */
#ifdef NH_GCONF
#include <gconf/gconf-client.h>

static int __g_gconf_klog = 0;

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

void gconf_client_set(GConfClient* client, const gchar* key,
		const GConfValue* val, GError** err)
{
	char buf[256], *vstr;

	static void *(*realfunc)(GConfClient*, const gchar*, const GConfValue*, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_client_set");

	realfunc(client, key, val, err);

	vstr = entry_value(val, buf, sizeof(buf));
	if (__g_gconf_klog)
		klogf("NEMOHOOK: gconf_sources_query_value: key: <%s>, vstr:<%s>\n", key, vstr);
	if (vstr != buf)
		free(vstr);
}

GConfValue* gconf_client_get(GConfClient* client, const gchar* key, GError** err)
{
	char buf[256], *val;

	static GConfValue *(*realfunc)(GConfClient*, const gchar*, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_client_get");

	GConfValue *ret = realfunc(client, key, err);

	val = entry_value(ret, buf, sizeof(buf));
	if (__g_gconf_klog)
		klogf("NEMOHOOK: gconf_client_get: key: <%s>, val:<%s>\n", key, val);
	if (val != buf)
		free(val);

	return ret;
}

#endif

/*-----------------------------------------------------------------------
 * ioctl - PI
 */
#ifdef NH_IOCTL
#include <sys/ioctl.h>

static int __g_ioctl_klog = 0;

int ioctl(int d, unsigned long int r, ...)
{
	static int (*realfunc)(int d, unsigned long int r, void*) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "ioctl");

	va_list args;
	void *argp;

	va_start(args, r);
	argp = va_arg(args, void *);
	va_end(args);

	int ret = realfunc(d, r, argp);
	if (__g_ioctl_klog)
		klogf("ioctl: d:%d, r:%08x, dir:%08x, type:%08x, nr:%08x, size:%08x, argp:%08x\n",
				d, r, _IOC_DIR(r), _IOC_TYPE(r), _IOC_NR(r), _IOC_SIZE(r), argp);

	return ret;
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
