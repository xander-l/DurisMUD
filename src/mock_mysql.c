#include "mock_mysql.h"

#include <string.h>

static const char *mock_mysql_last_error = "mock mysql: no error";

extern "C" {

MYSQL *mysql_init(MYSQL *mysql)
{
  if (mysql == NULL)
  {
    static MYSQL static_conn;
    static_conn.connected = 0;
    return &static_conn;
  }
  mysql->connected = 0;
  return mysql;
}

MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user,
                          const char *passwd, const char *db, unsigned int port,
                          const char *unix_socket, unsigned long clientflag)
{
  if (mysql == NULL)
    return NULL;
  mysql->connected = 1;
  return mysql;
}

int mysql_real_query(MYSQL *mysql, const char *stmt_str, unsigned long length)
{
  (void)mysql;
  (void)stmt_str;
  (void)length;
  return 0;
}

MYSQL_RES *mysql_use_result(MYSQL *mysql)
{
  (void)mysql;
  return NULL;
}

MYSQL_RES *mysql_store_result(MYSQL *mysql)
{
  static MYSQL_RES empty_result;
  (void)mysql;
  empty_result.num_rows = 0;
  return &empty_result;
}

my_ulonglong mysql_num_rows(MYSQL_RES *res)
{
  if (res == NULL)
    return 0;
  return res->num_rows;
}

unsigned int mysql_num_fields(MYSQL_RES *res)
{
  (void)res;
  return 0;
}

MYSQL_FIELD *mysql_fetch_fields(MYSQL_RES *res)
{
  (void)res;
  return NULL;
}

MYSQL_ROW mysql_fetch_row(MYSQL_RES *res)
{
  (void)res;
  return NULL;
}

void mysql_free_result(MYSQL_RES *res)
{
  (void)res;
}

unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from,
                                       unsigned long length)
{
  unsigned long out = 0;
  (void)mysql;
  if (to == NULL || from == NULL)
    return 0;
  for (unsigned long i = 0; i < length; i++)
  {
    char c = from[i];
    if (c == '\\' || c == '\'' || c == '\"')
    {
      to[out++] = '\\';
    }
    to[out++] = c;
  }
  to[out] = '\0';
  return out;
}

unsigned long long mysql_insert_id(MYSQL *mysql)
{
  (void)mysql;
  return 0;
}

const char *mysql_error(MYSQL *mysql)
{
  (void)mysql;
  return mock_mysql_last_error;
}

unsigned int mysql_errno(MYSQL *mysql)
{
  (void)mysql;
  return 0;
}

void mysql_close(MYSQL *mysql)
{
  if (mysql != NULL)
    mysql->connected = 0;
}

}
