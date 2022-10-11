#ifndef __QCOMMON_SQL_H__
#define __QCOMMON_SQL_H__

#include <sqlite3.h>
#include <stdbool.h>

#include <alias/cpp.h>

#define SQL_QUERY_BIND_ARGS_column_text(...)
#define SQL_QUERY_BIND_ARGS_column_int64(...)
#define SQL_QUERY_BIND_ARGS_bind_text(index, name) const char *name,
#define SQL_QUERY_BIND_ARGS(X) ALIAS_CPP_CAT(SQL_QUERY_BIND_ARGS_, X)

#define SQL_QUERY_BIND_column_text(...)
#define SQL_QUERY_BIND_column_int64(...)
#define SQL_QUERY_BIND_bind_text(index, name) sqlite3_bind_text(stmt, index + 1, name, -1, SQLITE_STATIC);
#define SQL_QUERY_BIND(X) ALIAS_CPP_CAT(SQL_QUERY_BIND_, X)

#define SQL_QUERY_CALLBACK_ARGS_bind_text(...)
#define SQL_QUERY_CALLBACK_ARGS_column_text(index, name) , const char *name
#define SQL_QUERY_CALLBACK_ARGS_column_int64(index, name) , int64_t name
#define SQL_QUERY_CALLBACK_ARGS(X) ALIAS_CPP_CAT(SQL_QUERY_CALLBACK_ARGS_, X)

#define SQL_QUERY_EXTRACT_bind_text(...)
#define SQL_QUERY_EXTRACT_column_text(index, name) const char *name = sqlite3_column_text(stmt, index);
#define SQL_QUERY_EXTRACT_column_int64(index, name) sqlite3_int64 name = sqlite3_column_int64(stmt, index);
#define SQL_QUERY_EXTRACT(X) ALIAS_CPP_CAT(SQL_QUERY_EXTRACT_, X)

#define SQL_QUERY_PASS_bind_text(...)
#define SQL_QUERY_PASS_column_text(index, name) , name
#define SQL_QUERY_PASS_column_int64(index, name) , name
#define SQL_QUERY_PASS(X) ALIAS_CPP_CAT(SQL_QUERY_PASS_, X)

#define SQL_QUERY(NAME, DB, SQL, ...)                                                                                  \
  sqlite3_stmt *NAME##_stmt(void) {                                                                                    \
    static sqlite3 *stmt_db = NULL;                                                                                    \
    static sqlite3_stmt *stmt = NULL;                                                                                  \
    if(DB != stmt_db) {                                                                                                \
      stmt = NULL;                                                                                                     \
    }                                                                                                                  \
    if(DB == NULL) {                                                                                                   \
      return NULL;                                                                                                     \
    }                                                                                                                  \
    if(stmt == NULL) {                                                                                                 \
      stmt_db = DB;                                                                                                    \
      int err;                                                                                                         \
      if(err = sqlite3_prepare_v2(DB, SQL, -1, &stmt, NULL)) {                                                         \
        Com_Error(ERR_FATAL, "SQL statement failed to prepare: %i", err);                                              \
      }                                                                                                                \
    }                                                                                                                  \
    return stmt;                                                                                                       \
  }                                                                                                                    \
  int NAME(ALIAS_CPP_EVAL_2(ALIAS_CPP_MAP(SQL_QUERY_BIND_ARGS, ##__VA_ARGS__)) void (*callback)(                       \
               void *ud ALIAS_CPP_EVAL(ALIAS_CPP_MAP(SQL_QUERY_CALLBACK_ARGS, ##__VA_ARGS__))),                        \
           void *ud) {                                                                                                 \
    sqlite3_stmt *stmt = NAME##_stmt();                                                                                \
    sqlite3_reset(stmt);                                                                                               \
    sqlite3_clear_bindings(stmt);                                                                                      \
    ALIAS_CPP_EVAL(ALIAS_CPP_MAP(SQL_QUERY_BIND, ##__VA_ARGS__))                                                       \
    int rows = 0;                                                                                                      \
    for(;;) {                                                                                                          \
      int code = sqlite3_step(stmt);                                                                                   \
      if(code == SQLITE_DONE) {                                                                                        \
        break;                                                                                                         \
      }                                                                                                                \
      if(code == SQLITE_ROW) {                                                                                         \
        ALIAS_CPP_EVAL(ALIAS_CPP_MAP(SQL_QUERY_EXTRACT, ##__VA_ARGS__))                                                \
        callback(ud ALIAS_CPP_EVAL(ALIAS_CPP_MAP(SQL_QUERY_PASS, ##__VA_ARGS__)));                                     \
        rows++;                                                                                                        \
        continue;                                                                                                      \
      }                                                                                                                \
      Com_Error(ERR_FATAL, "unexpected SQLite step result: %i", code);                                                 \
    }                                                                                                                  \
    return rows;                                                                                                       \
  }

#define SQL_DO(NAME, DB, SQL, ...)                                                                                     \
  sqlite3_stmt *NAME##_stmt(void) {                                                                                    \
    static sqlite3 *stmt_db = NULL;                                                                                    \
    static sqlite3_stmt *stmt = NULL;                                                                                  \
    if(DB != stmt_db) {                                                                                                \
      stmt = NULL;                                                                                                     \
    }                                                                                                                  \
    if(DB == NULL) {                                                                                                   \
      return NULL;                                                                                                     \
    }                                                                                                                  \
    if(stmt == NULL) {                                                                                                 \
      stmt_db = DB;                                                                                                    \
      int err;                                                                                                         \
      if(err = sqlite3_prepare_v2(DB, SQL, -1, &stmt, NULL)) {                                                         \
        Com_Error(ERR_FATAL, "SQL statement failed to prepare: %i", err);                                              \
      }                                                                                                                \
    }                                                                                                                  \
    return stmt;                                                                                                       \
  }                                                                                                                    \
  void NAME(ALIAS_CPP_EVAL_2(ALIAS_CPP_MAP(SQL_QUERY_BIND_ARGS, ##__VA_ARGS__))) {                                     \
    sqlite3_stmt *stmt = NAME##_stmt();                                                                                \
    sqlite3_reset(stmt);                                                                                               \
    sqlite3_clear_bindings(stmt);                                                                                      \
    ALIAS_CPP_EVAL(ALIAS_CPP_MAP(SQL_QUERY_BIND, ##__VA_ARGS__))                                                       \
    for(;;) {                                                                                                          \
      int code = sqlite3_step(stmt);                                                                                   \
      if(code == SQLITE_DONE) {                                                                                        \
        break;                                                                                                         \
      }                                                                                                                \
      if(code == SQLITE_ROW) {                                                                                         \
        continue;                                                                                                      \
      }                                                                                                                \
      Com_Error(ERR_FATAL, "unexpected SQLite step result: %i", code);                                                 \
    }                                                                                                                  \
  }

bool SQLite_exec(sqlite3 *db,                /* The database on which the SQL executes */
                 const char *zSql,           /* The SQL to be executed */
                 sqlite3_callback xCallback, /* Invoke this callback routine */
                 void *pArg                  /* First argument to xCallback() */
);

#endif