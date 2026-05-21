#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int append_to_logs(char *user_token) {
  FILE *log_file;
  char *log_filename = "/tmp/logs/users.log";

  // open the log file (create it if it doesn't exist)
  log_file = fopen(log_filename, "a");

  if (!log_file) {
    return -1; // Error opening log file
  }
  fprintf(log_file, "[+] New request to read secret by token: ");
  fprintf(log_file, user_token);
  fprintf(log_file, "\n");
  fclose(log_file);
  return 0; // Success
}

EMSCRIPTEN_KEEPALIVE
int is_token_valid(sqlite3 *db, char *token) {
  char query[] = "SELECT id FROM users WHERE token = ?";
  sqlite3_stmt *prepared_query = NULL;
  int rc;

  rc = sqlite3_prepare_v2(db, query, -1, &prepared_query, NULL);
  if (rc != SQLITE_OK) {
    return -1;
  }

  rc = sqlite3_bind_text(prepared_query, 1, token, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    sqlite3_finalize(NULL);
    return -1;
  }

  rc = sqlite3_step(prepared_query);
  // clean up the prepared statement
  sqlite3_finalize(prepared_query);

  // return 1 if user is found, 0 if not, -1 in case of error
  switch (rc) {
  case SQLITE_ROW:
    return 1;
  case SQLITE_DONE:
    return 0;
  default:
    return -1;
  }
}

EMSCRIPTEN_KEEPALIVE
const char *get_user_secret(sqlite3 *db, char *token, int *status) {
  // the 'Secret:' is a flag to make dumping the returned value from memory
  // easier
  char query[] = "SELECT CONCAT('Secret: ', secret), token FROM users";
  const char *secret = NULL;
  sqlite3_stmt *prepared_query;
  int rc;

  // default status to error
  if (status) {
    *status = -1;
  }

  // prepare the statement
  rc = sqlite3_prepare_v2(db, query, -1, &prepared_query, NULL);
  if (rc != SQLITE_OK) {
    return secret;
  }

  do {
    rc = sqlite3_step(prepared_query);

    if (rc != SQLITE_ROW) {
      if (rc != SQLITE_DONE) {
        *status = -1;
        sqlite3_finalize(prepared_query);
        return NULL;
      }
      // if we don't have a row, break the loop
      break;
    }

    // log the query execution
    if (append_to_logs(token) == -1) {
      EM_ASM({ console.error("Error appending to logs"); });
    }

    // retrieve the user secret
    const char *tmp_token =
        (const char *)sqlite3_column_text(prepared_query, 1);
    const char *tmp_secret =
        (const char *)sqlite3_column_text(prepared_query, 0);

    if (tmp_secret && strcmp(token, tmp_token) == 0) {
      secret = (char *)malloc(strlen(tmp_secret) + 1);

      if (secret) {
        // user and secret was found
        if (status) {
          *status = 1;
        }

        strcpy((char *)secret, tmp_secret);
        sqlite3_finalize(prepared_query);
        return secret;
      } else {
        *status = -1;
        sqlite3_finalize(prepared_query);
        return NULL;
      }
    }
  } while (rc == SQLITE_ROW);

  // no user was found
  if (status) {
    *status = 0;
  }

  sqlite3_finalize(prepared_query);
  return secret;
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
