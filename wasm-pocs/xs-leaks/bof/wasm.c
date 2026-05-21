#define PCRE2_CODE_UNIT_WIDTH 8
#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <pcre2.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_USERS 11
#define MAX_SECRET_LENGTH 40
#define MAX_SECRETS 5

int n_users = 0;
char users_secrets[MAX_USERS][MAX_SECRETS][MAX_SECRET_LENGTH] = {};

// regex for searching secrets
char search_pattern[20];

EMSCRIPTEN_KEEPALIVE
void add_user(char *user_secret, int *user_id) {
  // resetting the search regex each time a new user registers
  strcpy(search_pattern, ".*");

  // copying the new secret in the first slot available
  strcpy(users_secrets[n_users][0], user_secret);
  n_users++;

  if (user_id) {
    *user_id = n_users - 1; // returning the user ID of the new user
  }
}

EMSCRIPTEN_KEEPALIVE
bool add_secret(int user_id, int secret_offset, char *secret) {
  int i;

  // Validating the input
  if (user_id < 0 || user_id >= n_users) {
    return false;
  }

  // copying the secret to the first slot available
  if (secret_offset == -1) {
    for (int i = 0; i < MAX_SECRETS; i++) {
      if (users_secrets[user_id][i][0] == '\0') {
        strcpy(users_secrets[user_id][i], secret);
        break;
      }
    }
    // checking that the user hasn't reached the maximum amount of secrets
    if (i == MAX_SECRETS) {
      return false;
    }
  } else if (secret_offset < 0 || secret_offset >= MAX_SECRETS ||
             users_secrets[user_id][secret_offset][0] == '\0') {
    // checking that there is a secret to override
    return false;
  } else {
    // copying the secret to the specified slot
    strcpy(users_secrets[user_id][secret_offset], secret);
  }
  printf("Regex: %s\n", search_pattern);

  return true;
}

EMSCRIPTEN_KEEPALIVE
void get_secret(int user_id, char *search, int *status, char *found_secret) {
  char user_regex[100] = "", error_message[200] = "";
  int error_number;
  PCRE2_SIZE error_offset;
  pcre2_code *user_regex_compiled;
  pcre2_match_data *user_match_data;

  // default status is "error" (-2)
  if (*status) {
    *status = -2;
  }

  // validating the user ID
  if (user_id < 0 || user_id >= n_users) {
    return;
  }

  // creating the regex for searching with the user input and compiling it
  sprintf(user_regex, search_pattern, search);

  // compiling the regex
  user_regex_compiled =
      pcre2_compile((PCRE2_SPTR)user_regex, PCRE2_ZERO_TERMINATED, 0,
                    &error_number, &error_offset, NULL);

  if (user_regex_compiled == NULL) {
    PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(error_number, buffer, sizeof(buffer));
    printf("PCRE2 compile failed at offset %zu: %s\n", error_offset, buffer);
    *status = -2; // change status to "error" (-2)
    return;
  }

  // checking the secrets against the regex
  *status = 0; // reset status to "not found" (0)
  for (int i = 0; i < MAX_SECRETS; i++) {
    user_match_data =
        pcre2_match_data_create_from_pattern(user_regex_compiled, NULL);

    // checking if the secret matches the user regex
    if (pcre2_match(user_regex_compiled, users_secrets[user_id][i],
                    PCRE2_ZERO_TERMINATED, 0, 0, user_match_data, 0) >= 0) {
      *status = 1; // change status to "found" (1)
      strcpy(found_secret, users_secrets[user_id][i]);
      pcre2_match_data_free(user_match_data);
      break;
    }
    pcre2_match_data_free(user_match_data);
  }

  pcre2_code_free(user_regex_compiled);
  return;
}
