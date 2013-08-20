/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#define _GNU_SOURCE
#include <dlfcn.h>

/* LD_PRELOAD=/home/shit/malloc_hook/libprog2.so ./prog1 */

/*-----------------------------------------------------------------------
 * Dummy
 */
#include <stdio.h>
#include <stdint.h>

/*-----------------------------------------------------------------------
 * sqlite
 */
#include <sqlite3.h>

static void sqliteTrace(void *arg, const char *query)
{
	printf("sqliteTrace: <%s>\n", query);
}

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
	static int (*my_sqlite3_open)(const char*, sqlite3**) = NULL;
	if (!my_sqlite3_open)
		my_sqlite3_open = dlsym(RTLD_NEXT, "sqlite3_open");

	int ret = my_sqlite3_open(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);
	printf("NEMOHOOK: my_sqlite3_open: file:\"%s\", ret:%d\n", filename, ret);

	return ret;
}
int sqlite3_open16(const void *filename, sqlite3 **ppDb)
{
	static int (*my_sqlite3_open16)(const char*, sqlite3**) = NULL;
	if (!my_sqlite3_open16)
		my_sqlite3_open16 = dlsym(RTLD_NEXT, "sqlite3_open16");

	int ret = my_sqlite3_open16(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);
	printf("NEMOHOOK: my_sqlite3_open16: file:\"%s\", ret:%d\n", (char*)filename, ret);

	return ret;
}
int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
	static int (*my_sqlite3_open_v2)(const char*, sqlite3**, int, const char*) = NULL;
	if (!my_sqlite3_open_v2)
		my_sqlite3_open_v2 = dlsym(RTLD_NEXT, "sqlite3_open_v2");

	int ret = my_sqlite3_open_v2(filename, ppDb, flags, zVfs);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);
	printf("NEMOHOOK: my_sqlite3_open_v2: file:\"%s\", ret:%d\n", filename, ret);

	return ret;
}

/*-----------------------------------------------------------------------
 * dbus
 */
#if 0
dbus_bool_t bus_dispatch_matches (BusTransaction *transaction,
		DBusConnection *sender,
		DBusConnection *addressed_recipient,
		DBusMessage    *message,
		DBusError      *error)
{
}
#endif

/*-----------------------------------------------------------------------
 * gconf
 */

/*-----------------------------------------------------------------------
 * ioctl - PI
 */
#include <sys/ioctl.h>

int ioctl(int d, unsigned long int request, ...)
{
	static int (*my_ioctl)(int d, unsigned long int request, void*) = NULL;
	if (!my_ioctl)
		my_ioctl = dlsym(RTLD_NEXT, "ioctl");

	va_list args;
	void *argp;

	va_start(args, request);
	argp = va_arg(args, void *);
	va_end(args);

	int ret = my_ioctl(d, request, argp);
	printf("NEMOHOOK: ioctl: d:%d, request:%lu\n", d, request);

	return ret;
}

