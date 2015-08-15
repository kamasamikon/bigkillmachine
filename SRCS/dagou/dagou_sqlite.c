/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define DALOG_MODU_NAME "DASQLITE"
#include <dalog.h>

#define DAGOU_SQLITE

/*-----------------------------------------------------------------------
 * sqlite
 */
#ifdef DAGOU_SQLITE
#include <sqlite3.h>

static int is_skip_dalog()
{
	static int skip_dalog = -1;

	if (dagou_unlikely(skip_dalog == -1)) {
		if (getenv("DAGOU_SQLITE_SKIP"))
			skip_dalog = 1;
		else
			skip_dalog = 0;
	}
	return skip_dalog;
}

static void sqliteTrace(void *arg, const char *query)
{
	dalog_info("SQL:<%p>:<%s>\n", arg, query);
}

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (dagou_unlikely(!realfunc))
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open");

	dalog_setup();
	int ret = realfunc(filename, ppDb);
	if (dagou_likely(ret == SQLITE_OK))
		sqlite3_trace(*ppDb, sqliteTrace, *ppDb);

	if (dagou_unlikely(is_skip_dalog()))
		return ret;

	dalog_info("sqlite3_open: file:\"%s\", ret:%d, *ppDb:%p\n", filename, ret, *ppDb);
	printf("sqlite3_open: file:\"%s\", ret:%d, *ppDb:%p\n", filename, ret, *ppDb);
	return ret;
}
int sqlite3_open16(const void *filename, sqlite3 **ppDb)
{
	static int (*realfunc)(const char*, sqlite3**) = NULL;
	if (dagou_unlikely(!realfunc))
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open16");

	dalog_setup();
	int ret = realfunc(filename, ppDb);
	if (dagou_likely(ret == SQLITE_OK))
		sqlite3_trace(*ppDb, sqliteTrace, *ppDb);

	if (dagou_unlikely(is_skip_dalog()))
		return ret;

	dalog_info("sqlite3_open16: file:\"%s\", ret:%d, *ppDb:%p\n", (char*)filename, ret, *ppDb);
	printf("sqlite3_open16: file:\"%s\", ret:%d, *ppDb:%p\n", (char*)filename, ret, *ppDb);
	return ret;
}
int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
{
	static int (*realfunc)(const char*, sqlite3**, int, const char*) = NULL;
	if (dagou_unlikely(!realfunc))
		realfunc = dlsym(RTLD_NEXT, "sqlite3_open_v2");

	dalog_setup();
	int ret = realfunc(filename, ppDb, flags, zVfs);
	if (dagou_likely(ret == SQLITE_OK))
		sqlite3_trace(*ppDb, sqliteTrace, *ppDb);

	if (dagou_unlikely(is_skip_dalog()))
		return ret;

	dalog_info("sqlite3_open_v2: file:\"%s\", ret:%d, *ppDb:%p\n", filename, ret, *ppDb);
	printf("sqlite3_open_v2: file:\"%s\", ret:%d, *ppDb:%p\n", filename, ret, *ppDb);
	return ret;
}
#endif

