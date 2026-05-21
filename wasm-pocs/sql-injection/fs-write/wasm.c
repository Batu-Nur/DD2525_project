#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

EMSCRIPTEN_KEEPALIVE
const char *get_user_secret(sqlite3 *db, char *token, int *status) {
  char query[] = "SELECT secret FROM users WHERE token=?";
  const char *secret = NULL;
  sqlite3_stmt *prepared_query;
  int rc;

  // format string vulnerability
  printf("User token\n");
  printf(token);
  printf("\n");

  printf("Query: %s\n", query);

  // default status to error
  if (status) {
    *status = -1;
  }

  // prepare the statement
  rc = sqlite3_prepare_v2(db, query, -1, &prepared_query, NULL);
  if (rc != SQLITE_OK) {
    return secret;
  }

  // bind parameters
  rc = sqlite3_bind_text(prepared_query, 1, token, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    sqlite3_finalize(prepared_query);
    return secret;
  }

  rc = sqlite3_step(prepared_query);

  if (rc == SQLITE_ROW) {
    // retrieve the user secret
    const char *tmp_secret =
        (const char *)sqlite3_column_text(prepared_query, 0);

    if (tmp_secret) {
      secret = (char *)malloc(strlen(tmp_secret) + 1);
      if (secret) {
        strcpy((char *)secret, tmp_secret);
      } else {
        *status = -1;
        sqlite3_finalize(prepared_query);
        return NULL;
      }
    }

    // user and secret was found
    if (status) {
      *status = 1;
    }

    sqlite3_finalize(prepared_query);
    return secret;
  } else if (rc == SQLITE_DONE) {
    // no user was found
    if (status) {
      *status = 0;
    }

    sqlite3_finalize(prepared_query);
    return secret;
  } else {
    sqlite3_finalize(prepared_query);
    return secret;
  }
}

EMSCRIPTEN_KEEPALIVE
int execute_query(sqlite3 *db, char *query) {
  char *err_msg = 0;
  int rc = sqlite3_exec(db, query, 0, 0, &err_msg);

  if (rc != SQLITE_OK) {
    EM_ASM({ console.error("SQL error:", UTF8ToString($0)); }, err_msg);
    sqlite3_free(err_msg);
  }
  return rc;
}

EMSCRIPTEN_KEEPALIVE
int open_database(char *db_name, sqlite3 **db) {
  int rc = sqlite3_open_v2(db_name, db, SQLITE_OPEN_READWRITE, NULL);

  if (rc != SQLITE_OK) {
    EM_ASM(
        { console.error("Error opening database:", UTF8ToString($0)); },
        sqlite3_errmsg(*db));
    return 0;
  }
  return 1;
}

EMSCRIPTEN_KEEPALIVE
int close_db(sqlite3 *db) {
  int rc = sqlite3_close(db);

  if (rc != SQLITE_OK) {
    EM_ASM(
        { console.error("[!] Error closing the database:", UTF8ToString($0)); },
        sqlite3_errmsg(db));
    return 0;
  } else {
    return 1;
  }
}
