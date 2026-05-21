#include <emscripten.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NONCE_SIZE 8

char *generated_nonce = NULL;

EMSCRIPTEN_KEEPALIVE
char *generate_nonce() {
  int fd;
  char buffer[NONCE_SIZE * 2 + 1] = "";

  if (generated_nonce != NULL) {
    printf("Nonce already generated: %s\n", generated_nonce);
    return generated_nonce;
  }

  generated_nonce = malloc(NONCE_SIZE * 2 + 1);
  if (generated_nonce == NULL) {
    printf("Error allocating memory for nonce\n");
    return NULL;
  }

  // generate a cryptographically secure nonce
  fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    printf("Error reading from /dev/urandom\n");
    return NULL;
  }

  if (read(fd, buffer, sizeof(buffer)) < 0) {
    close(fd);
    printf("Error reading from /dev/urandom\n");
    return NULL;
  }
  close(fd);

  // reading NONCE_SIZE bytes from buffer
  for (int i = 0; i < NONCE_SIZE; i++) {
    sprintf(generated_nonce + i * 2, "%02x", buffer[i]);
  }
  generated_nonce[NONCE_SIZE * 2] = '\0'; // null-terminate the string

  return generated_nonce;
}

EMSCRIPTEN_KEEPALIVE
char *get_script(char *username, int *status, char *nonce) {
  char html_page_template[70] =
      "<script src=\"/welcome.js?username=#{ username }\" nonce=%s>";
  char html_page[500];
  char *username_copy = NULL;

  if (generated_nonce == NULL) {
    generated_nonce = generate_nonce();
  } else {
    free(generated_nonce);
  }

  username_copy = malloc(strlen(username) + 1);
  if (username_copy == NULL) {
    printf("Error allocating memory for username copy\n");
    if (status) {
      *status = -1; // defaults to error
    }
    return NULL;
  }
  strcpy(username_copy, username);
  strcpy(nonce, generated_nonce);

  printf("nonce: %s\n", nonce);

  if (*status) {
    *status = -1; // defaults to error
  }

  sprintf(html_page, html_page_template, nonce);

  return html_page;
}
