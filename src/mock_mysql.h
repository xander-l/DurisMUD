#ifndef MOCK_MYSQL_H
#define MOCK_MYSQL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long long my_ulonglong;

typedef struct mock_mysql_conn
{
  int connected;
} MYSQL;

typedef struct mock_mysql_res
{
  my_ulonglong num_rows;
} MYSQL_RES;

typedef struct mock_mysql_field
{
  const char *name;
} MYSQL_FIELD;

typedef char **MYSQL_ROW;

MYSQL *mysql_init(MYSQL *mysql);
MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user,
                          const char *passwd, const char *db, unsigned int port,
                          const char *unix_socket, unsigned long clientflag);
int mysql_real_query(MYSQL *mysql, const char *stmt_str, unsigned long length);
MYSQL_RES *mysql_use_result(MYSQL *mysql);
MYSQL_RES *mysql_store_result(MYSQL *mysql);
my_ulonglong mysql_num_rows(MYSQL_RES *res);
unsigned int mysql_num_fields(MYSQL_RES *res);
MYSQL_FIELD *mysql_fetch_fields(MYSQL_RES *res);
MYSQL_ROW mysql_fetch_row(MYSQL_RES *res);
void mysql_free_result(MYSQL_RES *res);
unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from,
                                       unsigned long length);
unsigned long long mysql_insert_id(MYSQL *mysql);
const char *mysql_error(MYSQL *mysql);
unsigned int mysql_errno(MYSQL *mysql);
void mysql_close(MYSQL *mysql);

#ifdef __cplusplus
}
#endif

#endif
