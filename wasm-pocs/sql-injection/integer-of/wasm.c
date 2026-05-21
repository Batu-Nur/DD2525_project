#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int id;
  char token[11];
  char secret[51];
} user_data;

typedef struct {
  user_data *users;
  int n_users;
} users;

static int parse_user(void *users_array, int n_column, char **column_text,
                      char **column_name) {
  // allocating the necessary memory for the returned users
  users *users_conv = (users *)users_array;
  users_conv->n_users = users_conv->n_users + 1;
  users_conv->users = (user_data *)realloc(
      users_conv->users, users_conv->n_users + sizeof(user_data));
  if (!users_conv->users) {
    EM_ASM({ console.error("Memory allocation failed for users array."); });
    return -1;
  }

  // copying user data inside the array
  users_conv->users[users_conv->n_users - 1].id = atoi(column_text[0]);
  strncpy(users_conv->users[users_conv->n_users - 1].token, column_text[1], 11);
  strncpy(users_conv->users[users_conv->n_users - 1].secret, column_text[2],
          51);

  return 0;
}

EMSCRIPTEN_KEEPALIVE
char *get_user_secret(sqlite3 *db, int user_id, int *status) {
  char *errmsg = NULL, *user_secret = NULL;
  users users = {0};
  int rc;

  // default status to error
  if (status) {
    *status = -1;
  }

  // executing query
  rc = sqlite3_exec(db, "SELECT * FROM users", parse_user, &users, &errmsg);
  if (rc != SQLITE_OK) {
    EM_ASM(
        { console.error("Error executing query:", UTF8ToString($0)); }, errmsg);
    sqlite3_free(errmsg);
    return "";
  }

  printf("%d\n", user_id);
  if (user_id < 0 || user_id >= users.n_users) {
    if (status) {
      *status = 0;
    }
    free(users.users);
    return NULL;
  }

  if (status) {
    *status = 1;
  }

  user_secret = (char *)malloc(51);
  strncpy(user_secret, users.users[user_id].secret, 51);

  free(users.users);

  return user_secret;
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
