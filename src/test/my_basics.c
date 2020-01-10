/*
  Copyright (C) 1995-2007 MySQL AB

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  There are special exceptions to the terms and conditions of the GPL
  as it is applied to this software. View the full text of the exception
  in file LICENSE.exceptions in the top-level directory of this software
  distribution.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "odbctap.h"

DECLARE_TEST(my_basics)
{
  SQLLEN nRowCount;

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_basic, t_basic_2");

  /* create the table 'myodbc3_demo_result' */
  ok_sql(hstmt,
         "CREATE TABLE t_basic (id INT PRIMARY KEY, name VARCHAR(20))");

  /* insert 3 rows of data */
  ok_sql(hstmt, "INSERT INTO t_basic VALUES (1,'foo'),(2,'bar'),(3,'baz')");

  /* update second row */
  ok_sql(hstmt, "UPDATE t_basic SET name = 'bop' WHERE id = 2");

  /* get the rows affected by update statement */
  ok_stmt(hstmt, SQLRowCount(hstmt, &nRowCount));
  is_num(nRowCount, 1);

  /* delete third row */
  ok_sql(hstmt, "DELETE FROM t_basic WHERE id = 3");

  /* get the rows affected by delete statement */
  ok_stmt(hstmt, SQLRowCount(hstmt, &nRowCount));
  is_num(nRowCount, 1);

  /* alter the table 't_basic' to 't_basic_2' */
  ok_sql(hstmt,"ALTER TABLE t_basic RENAME t_basic_2");

  /*
    drop the table with the original table name, and it should
    return error saying 'table not found'
  */
  expect_sql(hstmt, "DROP TABLE t_basic", SQL_ERROR);

 /* now drop the table, which is altered..*/
  ok_sql(hstmt, "DROP TABLE t_basic_2");

  /* free the statement cursor */
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  return OK;
}


DECLARE_TEST(t_max_select)
{
  SQLINTEGER num;
  SQLCHAR    szData[20];

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_max_select");

  ok_sql(hstmt, "CREATE TABLE t_max_select (a INT, b VARCHAR(30))");

  ok_stmt(hstmt, SQLPrepare(hstmt,
                            (SQLCHAR *)"INSERT INTO t_max_select VALUES (?,?)",
                            SQL_NTS));

  ok_stmt(hstmt, SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &num, 0, NULL));
  ok_stmt(hstmt, SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,
                                  SQL_CHAR, 0, 0, szData, sizeof(szData),
                                  NULL));

  for (num= 1; num <= 1000; num++)
  {
    sprintf((char *)szData, "MySQL%d", (int)num);
    ok_stmt(hstmt, SQLExecute(hstmt));
  }

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_RESET_PARAMS));
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_sql(hstmt, "SELECT * FROM t_max_select");

  is_num(myrowcount(hstmt), 1000);

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_UNBIND));
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_max_select");

  return OK;
}


/* Simple function to do basic ops with MySQL */
DECLARE_TEST(t_basic)
{
  SQLINTEGER nRowCount= 0, nInData= 1, nOutData;
  SQLCHAR szOutData[31];

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_myodbc");

  ok_sql(hstmt, "CREATE TABLE t_myodbc (a INT, b VARCHAR(30))");

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  /* DIRECT INSERT */
  ok_sql(hstmt, "INSERT INTO t_myodbc VALUES (10, 'direct')");

  /* PREPARE INSERT */
  ok_stmt(hstmt, SQLPrepare(hstmt,
                            (SQLCHAR *)
                            "INSERT INTO t_myodbc VALUES (?, 'param')",
                            SQL_NTS));

  ok_stmt(hstmt, SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &nInData, 0, NULL));

  for (nInData= 20; nInData < 100; nInData= nInData+10)
  {
    ok_stmt(hstmt, SQLExecute(hstmt));
  }

  /* FREE THE PARAM BUFFERS */
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_RESET_PARAMS));
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  /* FETCH RESULT SET */
  ok_sql(hstmt, "SELECT * FROM t_myodbc");

  ok_stmt(hstmt, SQLBindCol(hstmt, 1, SQL_C_LONG, &nOutData, 0, NULL));
  ok_stmt(hstmt, SQLBindCol(hstmt, 2, SQL_C_CHAR, szOutData, sizeof(szOutData),
                            NULL));

  nInData= 10;
  while (SQLFetch(hstmt) == SQL_SUCCESS)
  {
    is_num(nOutData, nInData);
    is_str(szOutData, nRowCount++ ? "param" : "direct", 5);
    nInData += 10;
  }

  is_num(nRowCount, (nInData - 10) / 10);

  /* FREE THE OUTPUT BUFFERS */
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_UNBIND));
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_myodbc");

  return OK;
}


DECLARE_TEST(t_nativesql)
{
  SQLCHAR    out[128], in[]= "SELECT * FROM venu";
  SQLINTEGER len;

  ok_con(hdbc, SQLNativeSql(hdbc, in, SQL_NTS, out, sizeof(out), &len));
  is_num(len, (SQLINTEGER) sizeof(in) - 1);

  /*
   The second call is to make sure the first didn't screw up the stack.
   (Bug #28758)
  */

  ok_con(hdbc, SQLNativeSql(hdbc, in, SQL_NTS, out, sizeof(out), &len));
  is_num(len, (SQLINTEGER) sizeof(in) - 1);

  return OK;
}


/**
  This just tests that we can connect, disconnect and connect a few times
  without anything blowing up.
*/
DECLARE_TEST(t_reconnect)
{
  SQLHDBC hdbc1;
  long i;

  for (i= 0; i < 10; i++)
  {
    ok_env(henv, SQLAllocConnect(henv, &hdbc1));
    ok_con(hdbc1, SQLConnect(hdbc1, mydsn, SQL_NTS, myuid, SQL_NTS,
                             mypwd, SQL_NTS));
    ok_con(hdbc1, SQLDisconnect(hdbc1));
    ok_con(hdbc1, SQLFreeConnect(hdbc1));
  }

  return OK;
}


/**
  Bug #19823: SQLGetConnectAttr with SQL_ATTR_CONNECTION_TIMEOUT works
  incorrectly
*/
DECLARE_TEST(t_bug19823)
{
  SQLHDBC hdbc1;
  SQLINTEGER timeout;

  ok_env(henv, SQLAllocConnect(henv, &hdbc1));

  /*
   This first connect/disconnect is just to work around a bug in iODBC's
   implementation of SQLSetConnectAttr. It is fixed in 3.52.6, but
   Debian/Ubuntu still ships 3.52.5 as of 2007-12-06.
  */
  ok_con(hdbc1, SQLConnect(hdbc1, mydsn, SQL_NTS, myuid, SQL_NTS,
                           mypwd, SQL_NTS));
  ok_con(hdbc1, SQLDisconnect(hdbc1));

  ok_con(hdbc1, SQLSetConnectAttr(hdbc1, SQL_ATTR_LOGIN_TIMEOUT,
                                  (SQLPOINTER)17, 0));
  ok_con(hdbc1, SQLSetConnectAttr(hdbc1, SQL_ATTR_CONNECTION_TIMEOUT,
                                  (SQLPOINTER)12, 0));

  ok_con(hdbc1, SQLConnect(hdbc1, mydsn, SQL_NTS, myuid, SQL_NTS,
                           mypwd, SQL_NTS));

  ok_con(hdbc1, SQLGetConnectAttr(hdbc1, SQL_ATTR_LOGIN_TIMEOUT,
                                  &timeout, 0, NULL));
  is_num(timeout, 17);

  /*
    SQL_ATTR_CONNECTION_TIMEOUT is always 0, because the driver does not
    support it and the driver just silently swallows any value given for it.
  */
  ok_con(hdbc1, SQLGetConnectAttr(hdbc1, SQL_ATTR_CONNECTION_TIMEOUT,
                                  &timeout, 0, NULL));
  is_num(timeout, 0);

  ok_con(hdbc1, SQLDisconnect(hdbc1));
  ok_con(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


/**
 Test that we can connect with UTF8 as our charset, and things work right.
*/
DECLARE_TEST(charset_utf8)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLCHAR conn[256], conn_out[256];
  SQLLEN len;
  SQLSMALLINT conn_out_len;

  /**
   Bug #19345: Table column length multiplies on size session character set
  */
  ok_sql(hstmt, "DROP TABLE IF EXISTS t_bug19345");
  ok_sql(hstmt, "CREATE TABLE t_bug19345 (a VARCHAR(10), b VARBINARY(10))");
  ok_sql(hstmt, "INSERT INTO t_bug19345 VALUES ('abc','def')");

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;CHARSET=utf8",
          mydsn, myuid, mypwd);
  if (mysock != NULL)
  {
    strcat((char *)conn, ";SOCKET=");
    strcat((char *)conn, (char *)mysock);
  }
  if (myport)
  {
    char pbuff[20];
    sprintf(pbuff, ";PORT=%d", myport);
    strcat((char *)conn, pbuff);
  }

  ok_env(henv, SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc1));

  ok_con(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  ok_con(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  ok_sql(hstmt1, "SELECT _latin1 0x73E36F207061756C6F");

  ok_stmt(hstmt1, SQLFetch(hstmt1));

  is_str(my_fetch_str(hstmt1, conn_out, 1), "s\xC3\xA3o paulo", 10);

  expect_stmt(hstmt1, SQLFetch(hstmt1), SQL_NO_DATA);

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  ok_stmt(hstmt1, SQLColumns(hstmt1, (SQLCHAR *)"test", SQL_NTS, NULL, 0,
                             (SQLCHAR *)"t_bug19345", SQL_NTS,
                             (SQLCHAR *)"%", 1));

  ok_stmt(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 7), 10);
  is_num(my_fetch_int(hstmt1, 8), 30);
  is_num(my_fetch_int(hstmt1, 16), 30);

  ok_stmt(hstmt1, SQLFetch(hstmt1));
  is_num(my_fetch_int(hstmt1, 7), 10);
  is_num(my_fetch_int(hstmt1, 8), 10);
  is_num(my_fetch_int(hstmt1, 16), 10);

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  /* Big5's 0xA4A4 becomes utf8's 0xE4B8AD */
  ok_sql(hstmt1, "SELECT _big5 0xA4A4");

  ok_stmt(hstmt1, SQLFetch(hstmt1));

  ok_stmt(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len));
  is_num(conn[0], 0xE4);
  is_num(len, 3);

  ok_stmt(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len));
  is_num(conn[0], 0xB8);
  is_num(len, 2);

  ok_stmt(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len));
  is_num(conn[0], 0xAD);
  is_num(len, 1);

  expect_stmt(hstmt1, SQLGetData(hstmt1, 1, SQL_C_CHAR, conn, 2, &len),
              SQL_NO_DATA_FOUND);

  expect_stmt(hstmt1, SQLFetch(hstmt1), SQL_NO_DATA_FOUND);

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  ok_con(hdbc1, SQLDisconnect(hdbc1));
  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_bug19345");

  return OK;
}


/**
 GBK is a fun character set -- it contains multibyte characters that can
 contain 0x5c ('\'). This causes escaping problems if the driver doesn't
 realize that we're using GBK. (Big5 is another character set with a similar
 issue.)
*/
DECLARE_TEST(charset_gbk)
{
  HDBC hdbc1;
  HSTMT hstmt1;
  SQLCHAR conn[256], conn_out[256];
  /*
    The fun here is that 0xbf5c is a valid GBK character, and we have 0x27
    as the second byte of an invalid GBK character. mysql_real_escape_string()
    handles this, as long as it knows the character set is GBK.
  */
  SQLCHAR str[]= "\xef\xbb\xbf\x27\xbf\x10";
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;CHARSET=gbk",
          mydsn, myuid, mypwd);
  if (mysock != NULL)
  {
    strcat((char *)conn, ";SOCKET=");
    strcat((char *)conn, (char *)mysock);
  }
  if (myport)
  {
    char pbuff[20];
    sprintf(pbuff, ";PORT=%d", myport);
    strcat((char *)conn, pbuff);
  }

  ok_env(henv, SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc1));

  ok_con(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));
  ok_con(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  ok_stmt(hstmt1, SQLPrepare(hstmt1, (SQLCHAR *)"SELECT ?", SQL_NTS));
  ok_stmt(hstmt1, SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_CHAR,
                                   SQL_CHAR, 0, 0, str, sizeof(str),
                                   NULL));

  ok_stmt(hstmt1, SQLExecute(hstmt1));

  ok_stmt(hstmt1, SQLFetch(hstmt1));

  is_str(my_fetch_str(hstmt1, conn_out, 1), str, sizeof(str));

  expect_stmt(hstmt1, SQLFetch(hstmt1), SQL_NO_DATA);

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));
  ok_con(hdbc1, SQLDisconnect(hdbc1));
  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


/**
  Bug #7445: MyODBC still doesn't support batch statements
*/
DECLARE_TEST(t_bug7445)
{
  SQLLEN nRowCount;
  SQLHENV    henv1;
  SQLHDBC    hdbc1;
  SQLHSTMT   hstmt1;

  SET_DSN_OPTION(1 << 26);

  alloc_basic_handles(&henv1, &hdbc1, &hstmt1);

  ok_sql(hstmt1, "DROP TABLE IF EXISTS t_bug7445");

  /* create the table 'myodbc3_demo_result' */
  ok_sql(hstmt1,
         "CREATE TABLE t_bug7445(name VARCHAR(20))");

  /* multi statement insert */
  ok_sql(hstmt1, "INSERT INTO t_bug7445 VALUES ('bogdan');"
                 "INSERT INTO t_bug7445 VALUES ('georg');"
                 "INSERT INTO t_bug7445 VALUES ('tonci');"
                 "INSERT INTO t_bug7445 VALUES ('jim')");

  ok_sql(hstmt1, "SELECT COUNT(*) FROM t_bug7445");

  /* get the rows affected by update statement */
  ok_stmt(hstmt1, SQLRowCount(hstmt1, &nRowCount));
  is_num(nRowCount, 1);

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  ok_sql(hstmt1, "DROP TABLE t_bug7445");

  free_basic_handles(&henv1, &hdbc1, &hstmt1);

  SET_DSN_OPTION(0);

  return OK;
}


/**
 Bug #30774: Username argument to SQLConnect used incorrectly
*/
DECLARE_TEST(t_bug30774)
{
  SQLHDBC hdbc1;
  SQLHSTMT hstmt1;
  SQLCHAR username[MAX_ROW_DATA_LEN+1]= {0};

  strcat((char *)username, (char *)myuid);
  strcat((char *)username, "!!!");

  ok_env(henv, SQLAllocConnect(henv, &hdbc1));
  ok_con(hdbc1, SQLConnect(hdbc1, mydsn, SQL_NTS,
                           username, strlen((char *)myuid),
                           mypwd, SQL_NTS));
  ok_con(hdbc1, SQLAllocStmt(hdbc1, &hstmt1));

  ok_sql(hstmt1, "SELECT USER()");
  ok_stmt(hstmt1, SQLFetch(hstmt1));
  my_fetch_str(hstmt1, username, 1);
  printMessage("username: %s", username);
  is(!strstr((char *)username, "!!!"));

  expect_stmt(hstmt1, SQLFetch(hstmt1), SQL_NO_DATA_FOUND);

  ok_stmt(hstmt1, SQLFreeStmt(hstmt1, SQL_DROP));

  ok_con(hdbc1, SQLDisconnect(hdbc1));
  ok_con(hdbc1, SQLFreeConnect(hdbc1));

  return OK;
}


/**
  Bug #30840: FLAG_NO_PROMPT doesn't do anything
*/
DECLARE_TEST(t_bug30840)
{
  HDBC hdbc1;
  SQLCHAR   conn[256], conn_out[256];
  SQLSMALLINT conn_out_len;

  if (using_dm(hdbc))
    skip("test does not work with all driver managers");

  sprintf((char *)conn, "DSN=%s;UID=%s;PASSWORD=%s;OPTION=16",
          mydsn, myuid, mypwd);
  if (mysock != NULL)
  {
    strcat((char *)conn, ";SOCKET=");
    strcat((char *)conn, (char *)mysock);
  }
  if (myport)
  {
    char pbuff[20];
    sprintf(pbuff, ";PORT=%d", myport);
    strcat((char *)conn, pbuff);
  }

  ok_env(henv, SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc1));

  ok_con(hdbc1, SQLDriverConnect(hdbc1, (HWND)1, conn, sizeof(conn),
                                 conn_out, sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_PROMPT));

  ok_con(hdbc1, SQLDisconnect(hdbc1));

  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


/**
  Bug #30983: SQL Statements limited to 64k
*/
DECLARE_TEST(t_bug30983)
{
  SQLCHAR buf[(80 * 1024) + 100]; /* ~80k */
  SQLCHAR *bufp = buf;
  SQLLEN buflen;
  int i, j;

  bufp+= sprintf((char *)bufp, "select '");

  /* fill 1k of each value */
  for (i= 0; i < 80; ++i)
    for (j= 0; j < 512; ++j, bufp += 2)
      sprintf((char *)bufp, "%02x", i);

  sprintf((char *)bufp, "' as val");

  ok_stmt(hstmt, SQLExecDirect(hstmt, buf, SQL_NTS));
  ok_stmt(hstmt, SQLFetch(hstmt));
  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, buf, 0, &buflen));
  is_num(buflen, 80 * 1024);
  return OK;
}


/*
   Test the output string after calling SQLDriverConnect
*/
DECLARE_TEST(t_driverconnect_outstring)
{
  HDBC hdbc1;
  SQLCHAR conn[256], conn_out[256], exp_out[256];
  SQLSMALLINT conn_out_len;
  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;CHARSET=utf8",
          mydsn, myuid, mypwd);
  if (mysock != NULL)
  {
    strcat((char *)conn, ";SOCKET=");
    strcat((char *)conn, (char *)mysock);
  }

  ok_env(henv, SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc1));

  ok_con(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, sizeof(conn), conn_out,
                                 sizeof(conn_out), &conn_out_len,
                                 SQL_DRIVER_NOPROMPT));

  sprintf((char *)exp_out, "DSN=%s;UID=%s", mydsn, myuid);
  if (mypwd && *mypwd)
  {
    strcat((char *)exp_out, ";PWD=");
    strcat((char *)exp_out, (char *)mypwd);
  }
  strcat((char *)exp_out, ";DATABASE=");
  ok_con(hdbc1, SQLGetConnectAttr(hdbc1, SQL_ATTR_CURRENT_CATALOG,
                                  exp_out + strlen((char *)exp_out), 100,
                                  NULL));

  if (mysock != NULL)
  {
    strcat((char *)exp_out, ";SOCKET=");
    strcat((char *)exp_out, (char *)mysock);
  }
  strcat((char *)exp_out, ";PORT=3306;CHARSET=utf8");

  printMessage("Output connection string: %s", conn_out);
  printMessage("Expected output   string: %s", exp_out);
  is_str(conn_out, exp_out, strlen((char *)conn_out));
  ok_con(hdbc1, SQLDisconnect(hdbc1));
  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));
  return OK;
}


DECLARE_TEST(setnames)
{
  expect_sql(hstmt, "SET NAMES utf8", SQL_ERROR);
  expect_sql(hstmt, "SeT NamES utf8", SQL_ERROR);
  expect_sql(hstmt, "   set names utf8", SQL_ERROR);
  expect_sql(hstmt, "	set names utf8", SQL_ERROR);
  return OK;
}


DECLARE_TEST(setnames_conn)
{
  HDBC hdbc1;
  SQLCHAR conn[256], conn_out[256];
  SQLSMALLINT conn_out_len;

  sprintf((char *)conn, "DSN=%s;UID=%s;PWD=%s;INITSTMT={set names utf8}",
          mydsn, myuid, mypwd);
  if (mysock != NULL)
  {
    strcat((char *)conn, ";SOCKET=");
    strcat((char *)conn, (char *)mysock);
  }

  ok_env(henv, SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc1));

  expect_dbc(hdbc1, SQLDriverConnect(hdbc1, NULL, conn, SQL_NTS, conn_out,
                                     sizeof(conn_out), &conn_out_len,
                                     SQL_DRIVER_NOPROMPT),
             SQL_ERROR);

  ok_con(hdbc1, SQLFreeHandle(SQL_HANDLE_DBC, hdbc1));

  return OK;
}


/**
 Bug #15601: SQLCancel does not work to stop a query on the database server
*/
#ifdef WIN32
DWORD WINAPI cancel_in_one_second(LPVOID arg)
{
  HSTMT hstmt= (HSTMT)arg;

  Sleep(1000);

  if (SQLCancel(hstmt) != SQL_SUCCESS)
    printMessage("SQLCancel failed!");

  return 0;
}


DECLARE_TEST(sqlcancel)
{
  HANDLE thread;
  DWORD waitrc;

  thread= CreateThread(NULL, 0, cancel_in_one_second, hstmt, 0, NULL);

  /* SLEEP(n) returns 1 when it is killed. */
  ok_sql(hstmt, "SELECT SLEEP(10)");
  ok_stmt(hstmt, SQLFetch(hstmt));
  is_num(my_fetch_int(hstmt, 1), 1);

  waitrc= WaitForSingleObject(thread, 10000);
  is(!(waitrc == WAIT_TIMEOUT));

  return OK;
}
#else
void *cancel_in_one_second(void *arg)
{
  HSTMT *hstmt= arg;

  sleep(1);

  if (SQLCancel(hstmt) != SQL_SUCCESS)
    printMessage("SQLCancel failed!");

  return NULL;
}

#include <pthread.h>

DECLARE_TEST(sqlcancel)
{
  pthread_t thread;

  pthread_create(&thread, NULL, cancel_in_one_second, hstmt);

  /* SLEEP(n) returns 1 when it is killed. */
  ok_sql(hstmt, "SELECT SLEEP(10)");
  ok_stmt(hstmt, SQLFetch(hstmt));
  is_num(my_fetch_int(hstmt, 1), 1);

  pthread_join(thread, NULL);

  return OK;
}
#endif


/**
Bug #32014: MyODBC / ADO Unable to open record set using dynamic cursor
*/
DECLARE_TEST(t_bug32014)
{
  SQLHENV     henv1;
  SQLHDBC     hdbc1;
  SQLHSTMT    hstmt1;
  SQLUINTEGER info;
  long        i=0;
  SQLSMALLINT value_len;

  long flags[]= { 0,
                  (131072L << 4)   /*FLAG_FORWARD_CURSOR*/,
                  32               /*FLAG_DYNAMIC_CURSOR*/,
                  (131072L << 4) | 32,
                  0 };

  long expectedInfo[]= { SQL_SO_FORWARD_ONLY|SQL_SO_STATIC,
                         SQL_SO_FORWARD_ONLY,
                         SQL_SO_FORWARD_ONLY|SQL_SO_STATIC|SQL_SO_DYNAMIC,
                         SQL_SO_FORWARD_ONLY };

  long expectedCurType[][4]= {
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_STATIC,        SQL_CURSOR_STATIC,          SQL_CURSOR_STATIC},
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_FORWARD_ONLY,  SQL_CURSOR_FORWARD_ONLY,    SQL_CURSOR_FORWARD_ONLY},
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_STATIC,        SQL_CURSOR_DYNAMIC,         SQL_CURSOR_STATIC},
      {SQL_CURSOR_FORWARD_ONLY, SQL_CURSOR_FORWARD_ONLY,  SQL_CURSOR_FORWARD_ONLY,    SQL_CURSOR_FORWARD_ONLY}};

  do
  {
    SET_DSN_OPTION(flags[i]);
    alloc_basic_handles(&henv1, &hdbc1, &hstmt1);

    printMessage("checking %d (%d)", i, flags[i]);

    /*Checking that correct info is returned*/

    ok_stmt(hstmt1, SQLGetInfo(hdbc1, SQL_SCROLL_OPTIONS,
            (SQLPOINTER) &info, sizeof(long), &value_len));
    is_num(info, expectedInfo[i]);

    /*Checking that correct cursor type is set*/

    ok_stmt(hstmt1, SQLSetStmtOption(hstmt1, SQL_CURSOR_TYPE
            , SQL_CURSOR_FORWARD_ONLY ));
    ok_stmt(hstmt1, SQLGetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            (SQLPOINTER) &info));
    is_num(info, expectedCurType[i][SQL_CURSOR_FORWARD_ONLY]);

    ok_stmt(hstmt1, SQLSetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            SQL_CURSOR_KEYSET_DRIVEN ));
    ok_stmt(hstmt1, SQLGetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            (SQLPOINTER) &info));
    is_num(info, expectedCurType[i][SQL_CURSOR_KEYSET_DRIVEN]);

    ok_stmt(hstmt1, SQLSetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            SQL_CURSOR_DYNAMIC ));
    ok_stmt(hstmt1, SQLGetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            (SQLPOINTER) &info));
    is_num(info, expectedCurType[i][SQL_CURSOR_DYNAMIC]);

    ok_stmt(hstmt1, SQLSetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            SQL_CURSOR_STATIC ));
    ok_stmt(hstmt1, SQLGetStmtOption(hstmt1, SQL_CURSOR_TYPE,
            (SQLPOINTER) &info));
    is_num(info, expectedCurType[i][SQL_CURSOR_STATIC]);

    free_basic_handles(&henv1, &hdbc1, &hstmt1);

  } while (flags[++i]);

  SET_DSN_OPTION(0);

  return OK;
}


/*
  Bug #10128 Error in evaluating simple mathematical expression
  ADO calls SQLNativeSql with a NULL pointer for the result length,
  but passes a non-NULL result buffer.
*/
DECLARE_TEST(t_bug10128)
{
  SQLCHAR *query= (SQLCHAR *) "select 1,2,3,4";
  SQLCHAR nativesql[1000];
  SQLINTEGER nativelen;
  SQLINTEGER querylen= (SQLINTEGER) strlen((char *)query);

  ok_con(hdbc, SQLNativeSql(hdbc, query, SQL_NTS, NULL, 0, &nativelen));
  is_num(nativelen, querylen);

  ok_con(hdbc, SQLNativeSql(hdbc, query, SQL_NTS, nativesql, 1000, NULL));
  is_str(nativesql, query, querylen + 1);

  return OK;
}


/**
 Bug #32727: Unable to abort distributed transactions enlisted in MSDTC
*/
DECLARE_TEST(t_bug32727)
{
  is(SQLSetConnectAttr(hdbc, SQL_ATTR_ENLIST_IN_DTC,
                       (SQLPOINTER)1, SQL_IS_UINTEGER) == SQL_ERROR);
  return OK;
}


/*
  Bug #28820: Varchar Field length is reported as larger than actual
*/
DECLARE_TEST(t_bug28820)
{
  SQLULEN length;
  SQLCHAR dummy[20];
  SQLSMALLINT i;

  ok_sql(hstmt, "drop table if exists t_bug28820");
  ok_sql(hstmt, "create table t_bug28820 ("
                "x varchar(90) character set latin1,"
                "y varchar(90) character set big5,"
                "z varchar(90) character set utf8)");

  ok_sql(hstmt, "select x,y,z from t_bug28820");

  for (i= 0; i < 3; ++i)
  {
    length= 0;
    ok_stmt(hstmt, SQLDescribeCol(hstmt, i+1, dummy, sizeof(dummy), NULL,
                                  NULL, &length, NULL, NULL));
    is_num(length, 90);
  }

  ok_sql(hstmt, "drop table if exists t_bug28820");
  return OK;
}


/*
  Bug #31959 - Allows dirty reading with SQL_TXN_READ_COMMITTED
               isolation through ODBC
*/
DECLARE_TEST(t_bug31959)
{
  SQLCHAR level[50] = "uninitialized";
  SQLINTEGER i;
  SQLINTEGER levelid[] = {SQL_TXN_SERIALIZABLE, SQL_TXN_REPEATABLE_READ,
                          SQL_TXN_READ_COMMITTED, SQL_TXN_READ_UNCOMMITTED};
  SQLCHAR *levelname[] = {(SQLCHAR *)"SERIALIZABLE",
                          (SQLCHAR *)"REPEATABLE-READ",
                          (SQLCHAR *)"READ-COMMITTED",
                          (SQLCHAR *)"READ-UNCOMMITTED"};

  ok_stmt(hstmt, SQLPrepare(hstmt,
                            (SQLCHAR *)"select @@tx_isolation", SQL_NTS));

  /* check all 4 valid isolation levels */
  for(i = 3; i >= 0; --i)
  {
    ok_con(hdbc, SQLSetConnectAttr(hdbc, SQL_ATTR_TXN_ISOLATION,
                                   (SQLPOINTER)levelid[i], 0));
    ok_stmt(hstmt, SQLExecute(hstmt));
    ok_stmt(hstmt, SQLFetch(hstmt));
    ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, level, 50, NULL));
    is_str(level, levelname[i], strlen((char *)levelname[i]));
    printMessage("Level = %s\n", level);
    ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));
  }

  /* check invalid value (and corresponding SQL state) */
  is(SQLSetConnectAttr(hdbc, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER)999, 0) ==
     SQL_ERROR);
  {
  SQLCHAR     sql_state[6];
  SQLINTEGER  err_code= 0;
  SQLCHAR     err_msg[SQL_MAX_MESSAGE_LENGTH]= {0};
  SQLSMALLINT err_len= 0;

  memset(err_msg, 'C', SQL_MAX_MESSAGE_LENGTH);
  SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, 1, sql_state, &err_code, err_msg,
                SQL_MAX_MESSAGE_LENGTH - 1, &err_len);

  is_str(sql_state, (SQLCHAR *)"HY024", 5);
  }

  return OK;
}


BEGIN_TESTS
  ADD_TEST(my_basics)
  ADD_TEST(t_max_select)
  ADD_TEST(t_basic)
  ADD_TEST(t_nativesql)
#ifndef NO_DRIVERMANAGER
  ADD_TEST(t_reconnect)
  ADD_TEST(t_bug19823)
#endif
  ADD_TEST(charset_utf8)
  ADD_TEST(charset_gbk)
  ADD_TEST(t_bug7445)
  ADD_TEST(t_bug30774)
  ADD_TEST(t_bug30840)
  ADD_TEST(t_bug30983)
  /* TODO fix this test create a comparable output string */
  ADD_TODO(t_driverconnect_outstring)
  ADD_TEST(setnames)
  ADD_TEST(setnames_conn)
  ADD_TEST(sqlcancel)
  ADD_TEST(t_bug32014)
  ADD_TEST(t_bug10128)
  ADD_TEST(t_bug32727)
  ADD_TEST(t_bug28820)
  ADD_TEST(t_bug31959)
END_TESTS


RUN_TESTS
