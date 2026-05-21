#include <emscripten.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

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
int initialize_db(sqlite3 *db) {
  // query to initialize the db
  char *sql = "CREATE TABLE IF NOT EXISTS users ("
              "  id INTEGER PRIMARY KEY,"
              "  username VARCHAR(20) NOT NULL UNIQUE,"
              "  password VARCHAR(50) NOT NULL,"
              "  secret VARCHAR(50) NOT NULL"
              ");"
              "INSERT INTO users (id, username, password, secret) VALUES "
              "(1, 'admin', 'securepassword', 'supersecretinformation');"
              "INSERT INTO users (id, username, password, secret) VALUES "
              "(2, 'user', 'password', 'notsosecretinformation');";

  int rc = execute_query(db, sql);
  if (rc != SQLITE_OK) {
    EM_ASM(
        { console.error("Error initializing database:", UTF8ToString($0)); },
        sqlite3_errmsg(db));
    return 0;
  }
  return 1;
}

EMSCRIPTEN_KEEPALIVE
int open_database(char *db_name, sqlite3 **db) {
  int rc = sqlite3_open_v2(db_name, db,
                           SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);

  if (rc != SQLITE_OK) {
    EM_ASM(
        { console.error("Error opening database:", UTF8ToString($0)); },
        sqlite3_errmsg(*db));
    return 0;
  }
  return 1;
}

EMSCRIPTEN_KEEPALIVE
char *get_user_secret(sqlite3 *db, char *username, char *password,
                      int *status) {
  char query[] = "SELECT * FROM users WHERE username = ? AND password = ?";
  char user[21] = "";
  char *secret = NULL;
  sqlite3_stmt *prepared_query;
  int rc;

  // default status to error
  if (status) {
    *status = -1;
  }

  // buffer overflow
  strcpy(user, username);

  EM_ASM({ console.log("[+] Query: ", UTF8ToString($0)); }, query);

  // prepare the statement
  rc = sqlite3_prepare_v2(db, query, -1, &prepared_query, NULL);
  if (rc != SQLITE_OK) {
    EM_ASM(
        {
          console.error("[!] Error while crafting prepared statement: ",
                        UTF8ToString($0));
        },
        sqlite3_errmsg(db));
    return secret;
  }

  // bind parameters
  rc = sqlite3_bind_text(prepared_query, 1, user, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    EM_ASM(
        {
          console.error(
              "[!] Error while binding parameter of a prepared statement: ",
              UTF8ToString($0));
        },
        sqlite3_errmsg(db));
    sqlite3_finalize(prepared_query);
    return secret;
  }

  rc = sqlite3_bind_text(prepared_query, 2, password, -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    EM_ASM(
        {
          console.error(
              "[!] Error while binding parameter of a prepared statement: ",
              UTF8ToString($0));
        },
        sqlite3_errmsg(db));
    sqlite3_finalize(prepared_query);
    return secret;
  }

  rc = sqlite3_step(prepared_query);

  if (rc == SQLITE_ROW) {
    // retrieve the user secret
    const unsigned char *secret_value = sqlite3_column_text(prepared_query, 3);

    // user and secret was found
    if (status) {
      *status = 1;
    }

    if (secret_value) {
      // allocate the memory to return the pointer to the secret
      size_t secret_len = strlen((const char *)secret_value) + 1;
      secret = (char *)malloc(secret_len);

      // error allocating memory
      if (!secret) {
        *status = -1;
        EM_ASM({ console.error("[!] Error allocating memory for secret"); });
        return secret;
      }

      strcpy(secret, (const char *)secret_value);
    }

    // sqlite3_finalize(prepared_query);
    return secret;
  } else if (rc == SQLITE_DONE) {
    // no user was found
    if (status) {
      *status = 0;
    }

    sqlite3_finalize(prepared_query);
    return secret;
  } else {
    // an error occurred
    EM_ASM(
        {
          console.error("[!] Error while executing prepared statement: ",
                        UTF8ToString($0));
        },
        sqlite3_errmsg(db));
    sqlite3_finalize(prepared_query);
    return secret;
  }
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
