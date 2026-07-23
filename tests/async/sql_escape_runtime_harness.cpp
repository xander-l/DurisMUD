/*
 * Runtime boundary test for the production escape_str() implementation.
 *
 * From the repository root:
 *   g++ -std=c++20 -Wno-write-strings -DTEST_MUD -D__NO_TESTS__ \
 *     -DCHAOS_MUD=1 -ffunction-sections -fdata-sections \
 *     -I src -I tests/async -I/usr/include/libxml2 -I/usr/include/mysql \
 *     -I src/ships -c src/sql.c -o /tmp/sql_escape_test_sql.o
 *   g++ -std=c++20 -ffunction-sections -fdata-sections \
 *     -I src -I/usr/include/mysql \
 *     -c tests/async/sql_escape_runtime_harness.cpp \
 *     -o /tmp/sql_escape_runtime_harness.o
 *   g++ -Wl,--gc-sections /tmp/sql_escape_runtime_harness.o \
 *     /tmp/sql_escape_test_sql.o -lmysqlclient -pthread \
 *     -o /tmp/sql_escape_runtime_harness
 *   /tmp/sql_escape_runtime_harness
 */

#include "sql.h"

#include <mysql/mysql.h>

#include <cstdio>
#include <string>

extern MYSQL *DB;

int main()
{
	DB = mysql_init(nullptr);
	if (!DB)
	{
		std::fprintf(stderr, "mysql_init failed\n");
		return 1;
	}

	// MySQL doubles each quote. This exceeds the old 65,536-byte static buffer.
	std::string input(40000, '\'');
	std::string escaped = escape_str(input.c_str());
	if (escaped.size() != 80000)
	{
		std::fprintf(stderr, "unexpected escaped size: %zu\n", escaped.size());
		mysql_close(DB);
		return 2;
	}
	if (escaped.compare(0, 4, "\\'\\'") != 0)
	{
		std::fprintf(stderr, "unexpected escape prefix\n");
		mysql_close(DB);
		return 3;
	}

	mysql_close(DB);
	DB = nullptr;
	std::puts("SQL escape runtime harness passed");
	return 0;
}
