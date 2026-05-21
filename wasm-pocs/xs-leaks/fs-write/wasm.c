#define PCRE2_CODE_UNIT_WIDTH 8
#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <pcre2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_USERS 11
#define MAX_SECRET_LENGTH 40
#define MAX_SECRETS 5

int n_users = 0;
char users_secrets[MAX_USERS][MAX_SECRETS][MAX_SECRET_LENGTH] = {};

EMSCRIPTEN_KEEPALIVE
void get_secret(int user_id, char *search, int *status, char *found_secret) {
  // regex for searching secrets
  char search_pattern[] = "%s.*";

  printf("search pattern: %s\n", search_pattern);

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
  char search_pattern_with_user_input[100];
  strcpy(search_pattern_with_user_input, search_pattern);

  printf("user regex: %s\n", search_pattern_with_user_input);

  // compiling the regex
  user_regex_compiled =
      pcre2_compile((PCRE2_SPTR)search_pattern_with_user_input,
                    PCRE2_ZERO_TERMINATED, 0, 0, 0, NULL);

  if (user_regex_compiled == NULL) {
    PCRE2_UCHAR buffer[256];
    printf("PCRE2 compile failed: %s\n", "Unknown error");
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

EMSCRIPTEN_KEEPALIVE
void add_user(char *user_secret, int *user_id) {
  // copying the new secret in the first slot available
  strncpy(users_secrets[n_users][0], user_secret, MAX_SECRET_LENGTH - 1);
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
        strncpy(users_secrets[user_id][i], secret, MAX_SECRET_LENGTH - 1);
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
    printf("Adding new secret: ");
    printf(secret);
    printf("\n");

    // copying the secret to the specified slot
    strncpy(users_secrets[user_id][secret_offset], secret,
            MAX_SECRET_LENGTH - 1);
  }

  return true;
}
