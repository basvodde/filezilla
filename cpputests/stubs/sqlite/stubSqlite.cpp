
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "sqlite3.h"

int sqlite3_open(const char *filename, sqlite3 **ppDb)
{
	FAIL("sqlite3_open");
	return 0;
}

int sqlite3_step(sqlite3_stmt*)
{
	FAIL("sqlite3_step");
	return 0;
}

int sqlite3_reset(sqlite3_stmt *pStmt)
{
	FAIL("sqlite3_reset");
	return 0;
}

sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*)
{
	FAIL("sqlite3_last_insert_rowid");
	return 0;
}

int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64)
{
	FAIL("sqlite3_bind_int64");
	return 0;
}

int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail)
{
	FAIL("sqlite3_prepare_v2");
	return 0;
}

int sqlite3_column_type(sqlite3_stmt*, int iCol)
{
	FAIL("sqlite3_column_type");
	return 0;
}

const void *sqlite3_column_text16(sqlite3_stmt*, int iCol)
{
	FAIL("sqlite3_column_text16");
	return NULL;
}

int sqlite3_column_int(sqlite3_stmt*, int iCol)
{
	FAIL("sqlite3_column_int");
	return 0;
}

int sqlite3_close(sqlite3 *)
{
	FAIL("sqlite3_close");
	return 0;
}

int sqlite3_bind_int(sqlite3_stmt*, int, int)
{
	FAIL("sqlite3_bind_int");
	return 0;
}

int sqlite3_bind_null(sqlite3_stmt*, int)
{
	FAIL("sqlite3_bind_null");
	return 0;
}

int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void(*)(void*))
{
	FAIL("sqlite3_bind_text");
	return 0;
}

int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*))
{
	FAIL("sqlite3_bind_text16");
	return 0;
}


int sqlite3_column_bytes16(sqlite3_stmt*, int iCol)
{
	FAIL("sqlite3_column_bytes16");
	return 0;
}

sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol)
{
	FAIL("sqlite3_column_int64");
	return 0;
}

int sqlite3_finalize(sqlite3_stmt *pStmt)
{
	FAIL("sqlite3_finalize");
	return 0;
}

int sqlite3_exec(sqlite3*, const char *sql, int (*callback)(void*,int,char**,char**), void *, char **errmsg)
{
	FAIL("sqlite3_exec");
	return 0;
}

