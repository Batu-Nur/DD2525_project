#include <emscripten.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NONCE_SIZE 8

char *generate_nonce(char *nonce) {
  int fd;
  char buffer[NONCE_SIZE * 2 + 1] = "";

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
    sprintf(nonce + i * 2, "%02x", buffer[i]);
  }
  nonce[NONCE_SIZE * 2] = '\0'; // null-terminate the string

  printf("Nonce generated: %s\n", nonce);
}

EMSCRIPTEN_KEEPALIVE
char *get_script(char *username, int *status, char *nonce) {
  char html_page_template[] =
      "<script src=\"/welcome.js?username=#{ username }\" nonce=%s>";
  char html_page[500];
  char generated_nonce[NONCE_SIZE * 2 + 1];
  char username_copy[20] = "";

  if (*status) {
    *status = -1; // defaults to error
  }

  generate_nonce(generated_nonce);
  if (generated_nonce == NULL) {
    return NULL;
  }

  printf("Username: %s\n", username);
  strcpy(nonce, generated_nonce);
  strcpy(username_copy, username);

  sprintf(html_page, html_page_template, generated_nonce);

  printf("HTML page generated: %s\n", html_page);
  return html_page;
}
