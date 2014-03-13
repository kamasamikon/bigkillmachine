/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define KMODU_NAME "NHSQLITE"
#include <hilda/klog.h>

#define NH_SQLITE

/*-----------------------------------------------------------------------
 * sqlite
 */
#ifdef NH_SQLITE
#include <sqlite3.h>

static int is_skip_klog()
{
	static int skip_klog = -1;

	if (skip_klog == -1) {
		if (getenv("NH_SQLITE_SKIP"))
			skip_klog = 1;
		else
			skip_klog = 0;
	}
	return skip_klog;
}

static void sqliteTrace(void *arg, const char *query)
{
	klog("SQL:<%p>:<%s>\n", arg, query);
}

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open");

	klogmon_init();
	int ret = realfunc(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, *ppDb);

	if (is_skip_klog())
		return ret;

	klog("sqlite3_open: file:\"%s\", ret:%d, *ppDb:%p\n", filename, ret, *ppDb);
	return ret;
}
int sqlite3_open16(const void *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open16");

	klogmon_init();
	int ret = realfunc(filename, ppDb);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, *ppDb);

	if (is_skip_klog())
		return ret;

	klog("sqlite3_open16: file:\"%s\", ret:%d, *ppDb:%p\n", (char*)filename, ret, *ppDb);
	return ret;
}
int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
	static int (*realfunc)(const char*, sqlite3**, int, const char*) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open_v2");

	klogmon_init();
	int ret = realfunc(filename, ppDb, flags, zVfs);
	if (ret == SQLITE_OK)
		sqlite3_trace(*ppDb, sqliteTrace, *ppDb);

	if (is_skip_klog())
		return ret;

	klog("sqlite3_open_v2: file:\"%s\", ret:%d, *ppDb:%p\n", filename, ret, *ppDb);
	return ret;
}
#endif

