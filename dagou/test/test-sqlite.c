/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include "sqlite3.h"

/* ENABLE FOR DEBUG PRINTS */
#if 0
#define PRINT_VERBOSE
#endif

#define BLOB_SIZE_DEFAULT         60000
#define BLOB_ITER_COUNT_DEFAULT   10
#define PAGE_SIZE_DEFAULT         1024

#define DB_NAME                     "test.db"
#define DB_NAME_WAL                 DB_NAME"-wal"

int s_seed_initialized = 0;

int timespec_diff(struct timespec time1, struct timespec time2)
{
	int timediff_ms = 0;

	timediff_ms = timediff_ms + 1000 *(time2.tv_sec - time1.tv_sec)
		+ (time2.tv_nsec - time1.tv_nsec)/1000000;

	return timediff_ms;
}

/*copied and modified from the sqlite command line utility (shell). */
static void printDbStats(sqlite3 *db, sqlite3_stmt *stmt, int bReset)
{
	int iCur;
	int iHiwtr;

	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_PAGECACHE_USED, &iCur, &iHiwtr, bReset);
	printf( "Number of Pcache Pages Used:         %d (max %d) pages\n", iCur, iHiwtr);

	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_PAGECACHE_OVERFLOW, &iCur, &iHiwtr, bReset);
	printf( "Number of Pcache Overflow Bytes:     %d (max %d) bytes\n", iCur, iHiwtr);

	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_SCRATCH_USED, &iCur, &iHiwtr, bReset);
	printf( "Number of Scratch Allocations Used:  %d (max %d)\n", iCur, iHiwtr);

	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_SCRATCH_OVERFLOW, &iCur, &iHiwtr, bReset);
	printf( "Number of Scratch Overflow Bytes:    %d (max %d) bytes\n", iCur, iHiwtr);
	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_MALLOC_SIZE, &iCur, &iHiwtr, bReset);
	printf( "Largest Allocation:                  %d bytes\n", iHiwtr);
	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_PAGECACHE_SIZE, &iCur, &iHiwtr, bReset);
	printf( "Largest Pcache Allocation:           %d bytes\n", iHiwtr);
	iHiwtr = iCur = -1;
	sqlite3_status(SQLITE_STATUS_SCRATCH_SIZE, &iCur, &iHiwtr, bReset);
	printf( "Largest Scratch Allocation:          %d bytes\n", iHiwtr);

	if( db )
	{
		iHiwtr = iCur = -1;
		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_USED, &iCur, &iHiwtr, bReset);
		printf( "Lookaside Slots Used:                %d (max %d)\n", iCur, iHiwtr);
		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_HIT, &iCur, &iHiwtr, bReset);
		printf( "Successful lookaside attempts:       %d\n", iHiwtr);
		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, &iCur, &iHiwtr, bReset);
		printf( "Lookaside failures due to size:      %d\n", iHiwtr);
		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, &iCur, &iHiwtr, bReset);
		printf( "Lookaside failures due to OOM:       %d\n", iHiwtr);
		iHiwtr = iCur = -1;
		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_USED, &iCur, &iHiwtr, bReset);
		printf( "Pager Heap Usage:                    %d bytes\n", iCur);
		iHiwtr = iCur = -1;
		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_HIT, &iCur, &iHiwtr, 1);
		printf( "Page cache hits:                     %d\n", iCur);
		iHiwtr = iCur = -1;
		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_MISS, &iCur, &iHiwtr, 1);
		printf( "Page cache misses:                   %d\n", iCur);
		iHiwtr = iCur = -1;
		sqlite3_db_status(db, SQLITE_DBSTATUS_SCHEMA_USED, &iCur, &iHiwtr, bReset);
		printf( "Schema Heap Usage:                   %d bytes\n", iCur);
		iHiwtr = iCur = -1;
		sqlite3_db_status(db, SQLITE_DBSTATUS_STMT_USED, &iCur, &iHiwtr, bReset);
		printf( "Statement Heap/Lookaside Usage:      %d bytes\n", iCur);
	}

	if( db && stmt )
	{
		iCur = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_FULLSCAN_STEP, bReset);
		printf( "Fullscan Steps:                      %d\n", iCur);
		iCur = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_SORT, bReset);
		printf( "Sort Operations:                     %d\n", iCur);
		iCur = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_AUTOINDEX, bReset);
		printf( "Autoindex Inserts:                   %d\n", iCur);
	}
}

static int write_blob_test(sqlite3 *pDB, int blobsize, int count, int *writeMs)
{
	int i = 0, status = 0;
	/* create variable blob data block */
	char* pBlob = (char*) malloc(blobsize);
	const char *insert_sql = "INSERT INTO blobTest(tag, tagValue) VALUES(?, ?);";
	sqlite3_stmt *pStmt = NULL;

	/* Performance */
	struct timespec time1 = {0},
			time2 = {0};
#ifdef PRINT_VERBOSE
	printf("\nStart write_blob_test():\n");
#endif
	/* fill it with some random data */
	memset(pBlob, '%', blobsize);

	/* prepare a SQL to an internal structure, this is same as "stored procedure" */
	status = sqlite3_prepare_v2(pDB, insert_sql, strlen(insert_sql), &pStmt, NULL );
#ifdef PRINT_VERBOSE
	printf ("\tsqlite3_prepare_v2 status: %d\n", status);
#endif
	/* Timespec 1 */
	clock_gettime(CLOCK_MONOTONIC, &time1);

	for(i = 0; i < count; i++)
	{

		sqlite3_bind_int(pStmt, 1, i);
		sqlite3_bind_blob(pStmt, 2, (const char*)pBlob, blobsize, SQLITE_STATIC );

		status = sqlite3_step( pStmt );
		if ( status == SQLITE_DONE || status == SQLITE_ROW )
		{
#ifdef PRINT_VERBOSE
			printf("\tsqlite3_step(%d) OK: %d\n", i, status);
#endif
			status = SQLITE_OK;
		}
		sqlite3_clear_bindings( pStmt );
		sqlite3_reset( pStmt );
		if ( status != SQLITE_OK )
		{
#ifdef PRINT_VERBOSE
			printf("\tINSERT VALUE ERROR(i=%d)!\n", i);
#endif
			break;
		}
	}

	/* Timespec 2 */
	clock_gettime(CLOCK_MONOTONIC, &time2);
	*writeMs = timespec_diff(time1, time2);
#ifdef PRINT_VERBOSE
	printf("\tComplete: %d(ms)\n", *writeMs);
#endif

	sqlite3_finalize(pStmt);
	if (pBlob)
		free(pBlob);

	return 0;
}

static int initRand()
{
	if (!s_seed_initialized)
	{
		srand(time(NULL));
		s_seed_initialized = 1;
	}
}

static void swap(int* pArray, int size, int i, int j)
{
	if (i < size && j < size)
	{
		int iVal = pArray[i];

		pArray[i] = pArray[j];
		pArray[j] = iVal;
	}
}

/* generate list of available numbers */
static void generateOrderedListNums(int* pRands, int size)
{
	int i = 0;

	for (i = 0; i < size; i++)
	{
		pRands[i] = i;
	}
}

/* implemention of the "Knuth Shuffle" as suggested by Durstenfield */
static int* getRandNums(int* pRands, int size)
{
	if (pRands)
	{
		int i = 0;
		initRand();

		generateOrderedListNums(pRands, size);

		/* shuffle the list */
		for (i = size; i > 0; i--)
		{
			int j = rand()%i;
			swap(pRands, size, j, i);
		}

#ifdef PRINT_VERBOSE
		/* print */
		printf("Randoms: {");
		for (i = 0; i < size; i++)
		{
			printf("%d,", pRands[i]);
		}
		printf("}\n");
#endif

		return pRands;
	}
	else
	{
		return NULL;
	}
}

static int read_blob_test(sqlite3 *pDB, int blobsize, int count, int *randNumsArray, int *readMs)
{
	int i = 0,
	    status = 0;
	char* pBlob = (char*) malloc(sizeof(char)*blobsize);
	const char *select_sql = "SELECT tag, tagValue FROM blobTest where tag = ?";
	sqlite3_stmt *pStmt = NULL;

	struct timespec time1 = {0},
			time2 = {0};
#ifdef PRINT_VERBOSE
	printf("Start read_blob_test():\n");
#endif

	/* prepare the statement */
	status = sqlite3_prepare_v2(pDB, select_sql, strlen(select_sql), &pStmt, NULL);
#ifdef PRINT_VERBOSE
	printf("\tsqlite3_prepare_v2 status: %d\n", status);
#endif

	clock_gettime(CLOCK_MONOTONIC, &time1);

	/* TODO: read the blobs in random order */
	for (i = 0; i < count; i++)
	{
		int tag = randNumsArray[i];
		/* TODO: perhaps bind to a random row val */
		sqlite3_bind_int(pStmt, 1, tag);  /* select based on row */

		status = sqlite3_step(pStmt);

		if ( status == SQLITE_ROW )
		{
			char *pRead = 0;
			int   selTag = sqlite3_column_int(pStmt, 0);
#ifdef PRINT_VERBOSE
			printf( "Got a row, tag=%d\n", selTag);
#endif
			pRead = sqlite3_column_text(pStmt, 1);
			/* copy the pointer to a buffer */
			memcpy(pBlob, pRead, blobsize);
		}
		else
		{
#ifdef PRINT_VERBOSE
			printf( "step return %d, break now.\n", status );
#endif
			break;
		}

		sqlite3_clear_bindings(pStmt);
		sqlite3_reset(pStmt);
	}

	clock_gettime(CLOCK_MONOTONIC, &time2);
	*readMs = timespec_diff(time1, time2);
#ifdef PRINT_VERBOSE
	printf("\tComplete: %d(ms)\n", *readMs);
#endif

	sqlite3_finalize(pStmt);

	if (pBlob)
		free(pBlob);

	return status;
}

static int print_file_sz( char *path )
{
	int status;
	struct stat stat_buf;
	status = stat(path, &stat_buf);
	if (status)
	{
		printf("stat(%s) failed : status %d\n", path, status);
		return 0;
	}
	else
	{
		printf("stat(%s) : size %d\n", path, stat_buf.st_size);
		return stat_buf.st_size;
	}
}

static int page_size_callback(void *data, int argc, char **values, char **names)
{
	int i;
	for(i=0; i<argc; i++)
	{
		printf("page_size output: <%s>\n", values[i] ? values[i] : "NULL");
	}
	return 0;
}

static int page_size_check(sqlite3 *sqlite_db)
{
	int status;
	printf("page_size_check: pid=0x%x,  sqlite_db=0x%p\n", getpid(), sqlite_db);
	if(sqlite_db)
	{
		status = sqlite3_exec(sqlite_db, "PRAGMA page_size", page_size_callback, 0, 0);
		printf("page_size_check : pid=%d, status %d\n", getpid(), status);
	}
	else
		status = -1;
	return status;
}

static int read_uncommitted_callback(void *data, int argc, char **values, char **names)
{
	int i;
	for(i=0; i<argc; i++)
	{
		printf("read_uncommitted output: <%s>\n", values[i] ? values[i] : "NULL");
	}
	return 0;
}

static int read_uncommitted_check(sqlite3 *sqlite_db)
{
	int status;
	printf("read_uncommitted_check: pid=0x%x,  sqlite_db=0x%p\n", getpid(), sqlite_db);
	if(sqlite_db)
	{
		status = sqlite3_exec(sqlite_db, "PRAGMA read_uncommitted", read_uncommitted_callback, 0, 0);
		printf("read_uncommitted_check : pid=%d, status %d\n", getpid(), status);
	}
	else
		status = -1;
	return status;
}

static int journal_mode_callback(void *data, int argc, char **values, char **names)
{
	int i;
	for(i=0; i<argc; i++)
	{
		printf("journal_mode output: <%s>\n", values[i] ? values[i] : "NULL");
	}
	return 0;
}

static int journal_mode_check(sqlite3 *sqlite_db)
{
	int status;
	printf("journal_mode_check: pid=0x%x,  sqlite_db=0x%p\n", getpid(), sqlite_db);
	if(sqlite_db)
	{
		status = sqlite3_exec(sqlite_db, "PRAGMA journal_mode", journal_mode_callback, 0, 0);
		printf("journal_mode_check : pid=%d, status %d\n", getpid(), status);
	}
	else
		status = -1;
	return status;
}

static int setDBPragmas(sqlite3* pDB, int pageSize)
{
	char pragma_buf[128];
	int status = 0;

	sprintf(pragma_buf, "PRAGMA page_size=%d", pageSize);
	status = sqlite3_exec(pDB, pragma_buf, NULL, NULL, NULL);
	//page_size_check(pDB);
	status = sqlite3_exec( pDB, "PRAGMA read_uncommitted=1", NULL, NULL, NULL );
	//read_uncommitted_check(pDB);
	status = sqlite3_exec( pDB, "PRAGMA journal_mode=WAL", NULL, NULL, NULL );
	//journal_mode_check(pDB);

	return status;
}

static void runBlobInDbTest(int blobsize, int count, int pageSize, int *writeMs, int* readMs)
{
	int     status = 0;
	sqlite3 *pDB = NULL;

	/* delete old db files */
	unlink( DB_NAME );
	unlink( DB_NAME_WAL );

	status = sqlite3_open( DB_NAME, &pDB );
	status = setDBPragmas(pDB, pageSize);

	/* Create Tables */
	status = sqlite3_exec(pDB, "DROP TABLE IF EXISTS blobTest", NULL, NULL, NULL);
	status = sqlite3_exec(pDB, "CREATE TABLE blobTest(tag INTEGER NOT NULL, tagValue BLOB)", NULL, NULL, NULL);

	/* Write a variable blob size value */
	write_blob_test(pDB, blobsize, count, writeMs);

	/* Read test */
	int* pRandNums = (int*) malloc(count*sizeof(int));
	memset(pRandNums, 0, count);
	getRandNums(pRandNums, count);

	read_blob_test(pDB, blobsize, count, pRandNums, readMs);

	printf("\n");
	printDbStats(pDB, NULL, 0);

	if (pRandNums)
		free(pRandNums);

	/* Finish Connection */
	status = sqlite3_close(pDB);
}

/* Read/Write tests using an external file */
static void createFilePaths(char** ppFilePaths, int count)
{
#define BLOB_FILE_NAME_EXT        "blob"

	int i = 0;
	char nameBuf[32];

	for (i = 0; i < count; i++)
	{
		ppFilePaths[i] = (char*) malloc(sizeof(nameBuf));
		sprintf(ppFilePaths[i], "%d.%s", i, BLOB_FILE_NAME_EXT);
	}
}

static void freeFilePaths(char** ppFilePaths, int count)
{
	int i = 0;

	for (i = 0; i < count; i++)
	{
		if (ppFilePaths[i])
			free(ppFilePaths[i]);
	}
}

#define BYTES_CHECK(bytes_written, bytes)   ((bytes_written == bytes) ? 0 : -1)

static int create_and_write_blob(char* pFilename, char* data, int datasize)
{
	int bytes_written = 0;
	FILE *fp = NULL;

	if (pFilename && data)
	{
		if (NULL != (fp = fopen(pFilename, "w+")))
		{
			bytes_written = fwrite(data, 1, datasize, fp);
#ifdef PRINT_VERBOSE
			printf("\tbytes written: %d\n", bytes_written);
#endif
			fclose(fp);
		}
	}

	return bytes_written;
}

static int write_blob_with_filepath_test(sqlite3 *pDB, int blobsize, int count, char**ppFileNames, int*writeMs)
{
	int i = 0,
	    status = 0;
	/* create variable blob data block */
	char* pBlob = (char*) malloc(blobsize);
	const char *insert_sql = "INSERT INTO blobTest(tag, tagValue) VALUES(?, ?);";
	sqlite3_stmt *pStmt = NULL;
	/* Performance */
	struct timespec time1 = {0},
			time2 = {0};
#ifdef PRINT_VERBOSE
	printf("\nStart write_blob_with_filepath_test:\n");
#endif

	memset(pBlob, '%', blobsize);

	/* create a prepared statement */
	status = sqlite3_prepare_v2(pDB, insert_sql, strlen(insert_sql), &pStmt, NULL );
#ifdef PRINT_VERBOSE
	printf ("\tsqlite3_prepare_v2 status: %d\n", status);
#endif

	/* Timespec 1 */
	clock_gettime(CLOCK_MONOTONIC, &time1);

	for (i = 0; i < count; i++)
	{
		sqlite3_bind_int(pStmt, 1, i);

		/* write the blob to a file */
		status = BYTES_CHECK(create_and_write_blob(ppFileNames[i], pBlob, blobsize), blobsize);

		if(status != 0)
		{
#ifdef PRINT_VERBOSE
			printf("error creating file(%d): %d\n", i, status);
#endif
			break;
		}
		/* write the path to stmt */
		sqlite3_bind_blob(pStmt, 2, (const char*)ppFileNames[i], strlen(ppFileNames[i])+1, SQLITE_STATIC);

		/* write data to DB */
		status = sqlite3_step( pStmt );
		if ( status == SQLITE_DONE || status == SQLITE_ROW )
		{
#ifdef PRINT_VERBOSE
			printf("\tsqlite3_step(%d) OK: %d\n", i, status);
#endif
			status = SQLITE_OK;
		}
		sqlite3_clear_bindings( pStmt );
		sqlite3_reset( pStmt );
		if ( status != SQLITE_OK )
		{
#ifdef PRINT_VERBOSE
			printf("\tINSERT VALUE ERROR(i=%d)!\n", i);
#endif
			break;
		}
	}

	/* Timespec 2 */
	clock_gettime(CLOCK_MONOTONIC, &time2);
	*writeMs = timespec_diff(time1, time2);
#ifdef PRINT_VERBOSE
	printf("\tComplete: %d(ms)\n", *writeMs);
#endif

	sqlite3_finalize(pStmt);
	if (pBlob)
		free(pBlob);

	return status;
}

static void read_blob_with_filepath_test(sqlite3 *pDB, int blobsize, int count, int *readMs)
{
	int i = 0,
	    status = 0;
	char* pBlob = (char*) malloc((sizeof(char)*blobsize)+1);
	const char *select_sql = "SELECT tag, tagValue FROM blobTest where tag = ?";
	sqlite3_stmt *pStmt = NULL;
	int* pRandNums = NULL;

	struct timespec time1 = {0},
			time2 = {0};

	/* Read test */
	pRandNums = (int*) malloc(count*sizeof(int));
	memset(pRandNums, 0, count);
	getRandNums(pRandNums, count);
#ifdef PRINT_VERBOSE
	printf("Start read_blob_with_filapth_test():\n");
#endif

	/* prepare the statement */
	status = sqlite3_prepare_v2(pDB, select_sql, strlen(select_sql), &pStmt, NULL);
#ifdef PRINT_VERBOSE
	printf("\tsqlite3_prepare_v2 status: %d\n", status);
#endif

	clock_gettime(CLOCK_MONOTONIC, &time1);

	for (i = 0; i < count; i++)
	{
		int tag = pRandNums[i];
		sqlite3_bind_int(pStmt, 1, tag);  /* select based on row */

		status = sqlite3_step(pStmt);

		if ( status == SQLITE_ROW )
		{
			FILE *fp = NULL;
			int readTag = sqlite3_column_int(pStmt, 0);
			char* pFilename = sqlite3_column_text(pStmt, 1);

#ifdef PRINT_VERBOSE
			printf( "\tGot a row, tag=%d, file: %s\n", readTag, pFilename);
#endif
			/* read the file */
			if (pFilename)
			{
				int bytes = 0;
				if (NULL != (fp = fopen(pFilename, "r+")))
				{
					bytes = fread(pBlob, 1, blobsize, fp);
#ifdef PRINT_VERBOSE
					printf("\tread %d bytes\n", bytes);
#endif
				}
			}
		}
		else
		{
#ifdef PRINT_VERBOSE
			printf( "step return %d, break now.\n", status );
#endif
			break;
		}

		sqlite3_clear_bindings(pStmt);
		sqlite3_reset(pStmt);
	}

	clock_gettime(CLOCK_MONOTONIC, &time2);
	*readMs = timespec_diff(time1, time2);
#ifdef PRINT_VERBOSE
	printf("\tComplete: %d(ms)\n", *readMs);
#endif

	sqlite3_finalize(pStmt);

	if (pRandNums)
		free(pRandNums);
}

static void runBlobInFileTest(int blobsize, int count, int pageSize, int *writeMs, int *readMs)
{
	int     status = 0;
	sqlite3 *pDB = NULL;
	char** ppFileNames = 0;

	/* delete old db files */
	unlink( DB_NAME );
	unlink( DB_NAME_WAL );

	status = sqlite3_open( DB_NAME, &pDB );
	status = setDBPragmas(pDB, pageSize);

	/* Create Tables */
	status = sqlite3_exec(pDB, "DROP TABLE IF EXISTS blobTest", NULL, NULL, NULL);
	status = sqlite3_exec(pDB, "CREATE TABLE blobTest(tag INTEGER NOT NULL, tagValue BLOB)", NULL, NULL, NULL);

	/* Create a file store location */
	ppFileNames = (char**) malloc( count*(sizeof(char**)) );
	/* Generate file locations */
	createFilePaths(ppFileNames, count);
#if 0
	{
		int i = 0;
		printf("filepaths: \n");
		for(i=0; i < count; i++)
		{
			printf("%s\n", ppFileNames[i]);
		}
	}
#endif
	/* Write test */
	write_blob_with_filepath_test(pDB, blobsize, count, ppFileNames, writeMs);

#if 1
	/* Read test */
	read_blob_with_filepath_test(pDB, blobsize, count, readMs);
#endif

	printf("\n");
	printDbStats(pDB, NULL, 0);

	/* Finish Connection */
	status = sqlite3_close(pDB);

	freeFilePaths(ppFileNames, count);
	if (ppFileNames)
	{
		free(ppFileNames);
	}
}


/*
 * START MAIN TEST - blob in-database test, in external file
 *
 */
int main_sqlite( int argc, char **argv )
{
	int status;
	int blobsize = BLOB_SIZE_DEFAULT,
	    count = BLOB_ITER_COUNT_DEFAULT,
	    pageSize = PAGE_SIZE_DEFAULT;
	int inDbWritesMs = 0,
	    inDbReadsMs = 0,
	    extWritesMs = 0,
	    extReadsMs = 0;

	printf("db_blob_wrt_test:\n");
	/* Initialize Stdin parameters */
	if (argc > 1)
	{
		blobsize = atoi(argv[1]);
	}
	if (argc > 2)
	{
		count = atoi(argv[2]);
	}
	if (argc > 3)
	{
		pageSize = atoi(argv[3]);
	}

	printf("### blobsize=%d, count=%d, page_size=%d\n", blobsize, count, pageSize);

	printf( "sqlite3_libversion_number %d, sqlite3_libversion %s, sqlite3_sourceid %s\n", sqlite3_libversion_number(), sqlite3_libversion(), sqlite3_sourceid() );

#ifdef ENABLE_SHARED_CACHE
	status = sqlite3_enable_shared_cache(1);
#endif

	/* remove any remnant files */
	system("rm *.blob");

#if 1
	/* Blob In Database test */
	runBlobInDbTest(blobsize, count, pageSize, &inDbWritesMs, &inDbReadsMs);
#endif

#if 1 /* Testing */
	{
		int i = 0;
		/* Create a file store location */
		char** ppFileNames = (char**) malloc(sizeof(char**)*count);

		/* Generate file locations */
		createFilePaths(ppFileNames, count);

		printf("filepaths:\n");
		for(i=0; i < count; i++)
		{
			printf("filename: %s\n", ppFileNames[i]);
		}
	}
#endif

#if 1
	/* Start not in-db test */
	runBlobInFileTest(blobsize, count, pageSize, &extWritesMs, &extReadsMs);
#endif

	/* Summarize inDbWrites, inDbReads, sepFileWrites, sepFileReads */
	printf("\nExternal(wr,rd) vs internal(wr,rd) BLOBs summary: external(%d,%d)(ms) : internal(%d,%d)(ms) : ratio(ext wr/int wr): %f ratio(ext rd/int rd): %f\n\n",
			extWritesMs, extReadsMs, inDbWritesMs, inDbReadsMs, ((float)extWritesMs/(float)inDbWritesMs), ((float)extReadsMs/(float)inDbReadsMs));

	return 0;
}
