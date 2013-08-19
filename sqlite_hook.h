/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __SQLITE3_HOOK_H__
#define __SQLITE3_HOOK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sqlite3.h>
#include <syslog.h>

static char *sqlite_error(int error)
{
    switch (error) {
    case SQLITE_OK:
        return "SQLITE_OK";
    case SQLITE_ERROR:
        return "SQLITE_ERROR";
    case SQLITE_INTERNAL:
        return "SQLITE_INTERNAL";
    case SQLITE_PERM:
        return "SQLITE_PERM";
    case SQLITE_ABORT:
        return "SQLITE_ABORT";
    case SQLITE_BUSY:
        return "SQLITE_BUSY";
    case SQLITE_LOCKED:
        return "SQLITE_LOCKED";
    case SQLITE_NOMEM:
        return "SQLITE_NOMEM";
    case SQLITE_READONLY:
        return "SQLITE_READONLY";
    case SQLITE_INTERRUPT:
        return "SQLITE_INTERRUPT";
    case SQLITE_IOERR:
        return "SQLITE_IOERR";
    case SQLITE_CORRUPT:
        return "SQLITE_CORRUPT";
    case SQLITE_NOTFOUND:
        return "SQLITE_NOTFOUND";
    case SQLITE_FULL:
        return "SQLITE_FULL";
    case SQLITE_CANTOPEN:
        return "SQLITE_CANTOPEN";
    case SQLITE_PROTOCOL:
        return "SQLITE_PROTOCOL";
    case SQLITE_EMPTY:
        return "SQLITE_EMPTY";
    case SQLITE_SCHEMA:
        return "SQLITE_SCHEMA";
    case SQLITE_TOOBIG:
        return "SQLITE_TOOBIG";
    case SQLITE_CONSTRAINT:
        return "SQLITE_CONSTRAINT";
    case SQLITE_MISMATCH:
        return "SQLITE_MISMATCH";
    case SQLITE_MISUSE:
        return "SQLITE_MISUSE";
    case SQLITE_NOLFS:
        return "SQLITE_NOLFS";
    case SQLITE_AUTH:
        return "SQLITE_AUTH";
    case SQLITE_FORMAT:
        return "SQLITE_FORMAT";
    case SQLITE_RANGE:
        return "SQLITE_RANGE";
    case SQLITE_NOTADB:
        return "SQLITE_NOTADB";
    case SQLITE_ROW:
        return "SQLITE_ROW";
    case SQLITE_DONE:
        return "SQLITE_DONE";
    default:
        return "SQLITE_ZBD";
    }
}

static void sqlite3_dump_result(sqlite3_stmt *stmt, int rc)
{
	int col, cols, columnType;

	char buffer[4096];
	int ofs;

	if (rc != SQLITE_ROW)
		return;

	cols = sqlite3_column_count(stmt);
	for (col = 0; col < cols; col++) {
		ofs = 0;

		columnType = sqlite3_column_type(stmt, col);
		/* TODO: print the result into buffer first */
		switch(columnType)
		{
		case SQLITE_INTEGER:
		case SQLITE_FLOAT:
			syslog(LOG_ERR, "shit>>> [%d] %d.\n", col, sqlite3_column_double(stmt, col));
			break;

		case SQLITE_TEXT:
			syslog(LOG_ERR, "shit>>> [%d] %s.\n", col, sqlite3_column_text(stmt, col));
			break;

		case SQLITE_NULL:
			syslog(LOG_ERR, "shit>>> [%d] NULL.\n", col);
			break;

		case SQLITE_BLOB:
			syslog(LOG_ERR, "shit>>> [%d] BLOB.\n", col);
			break;

		default:
			syslog(LOG_ERR, "shit>>> [%d] UNKNOWN.\n", col);
			break;
		}
	}
}

static int nemo_sqlite3_step(sqlite3_stmt* stmt)
{
	int ret;

	ret = sqlite3_step(stmt);

#if 0
	syslog(LOG_ERR, "sqlite3_step: return: \"%s\", stmt: \"%s\"\n",
			sqlite_error(ret), sqlite3_sql(stmt));
#endif

	return ret;
}

#if 0
static void sqliteTrace(void *arg, const char *query)
{
	printf("sqliteTrace: query: <%s>\n", query);
}

int main (int argc, char *argv[])
{
	sqlite3 *db = sqlite3_open(argv[1]);
	sqlite3_trace(db, sqliteTrace, NULL);
}
#endif

#define sqlite3_step nemo_sqlite3_step

#ifdef __cplusplus
}
#endif
#endif /* __SQLITE3_HOOK_H__ */

