/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define KMODU_NAME "NHSQLITE"
#include <klog.h>

#define NH_SQLITE

/*-----------------------------------------------------------------------
 * sqlite
 */
#ifdef NH_SQLITE
#include <sqlite3.h>

static void sqliteTrace(void *arg, const char *query)
{
	klog("SQL: <%s>\n", query);
}

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open");

	int ret = realfunc(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, NULL);

	klog("NEMOHOOK: sqlite3_open: file:\"%s\", ret:%d\n", filename, ret);

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
	klog("NEMOHOOK: sqlite3_open16: file:\"%s\", ret:%d\n", (char*)filename, ret);

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
	klog("NEMOHOOK: sqlite3_open_v2: file:\"%s\", ret:%d\n", filename, ret);

	return ret;
}
#endif

