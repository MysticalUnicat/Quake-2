#include "qcommon.h"
#include "sql.h"

bool SQLite_exec(sqlite3 *db,                /* The database on which the SQL executes */
                 const char *zSql,           /* The SQL to be executed */
                 sqlite3_callback xCallback, /* Invoke this callback routine */
                 void *pArg                  /* First argument to xCallback() */
) {
  int err;
  char *errmsg;
  if(err = sqlite3_exec(db, zSql, xCallback, pArg, &errmsg)) {
    Com_Printf("SQLite exec error \"%s\" %i %s", zSql, err, errmsg);
  }
  if(errmsg) {
    sqlite3_free(errmsg);
  }
  return err == 0;
}
