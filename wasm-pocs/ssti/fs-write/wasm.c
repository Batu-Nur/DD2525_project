#include <emscripten.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define NONCE_SIZE 8

EMSCRIPTEN_KEEPALIVE
char *generate_nonce() {
  char generated_nonce[NONCE_SIZE * 2 + 1] = "aaaaaaaaaaaa";
  int fd;
  char buffer[NONCE_SIZE * 2 + 1] = "";

  printf("generated nonce: %s\n", generated_nonce);

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

  return generated_nonce;
}

EMSCRIPTEN_KEEPALIVE
char *get_script(char *username, int *status, char *nonce) {
  char html_page_template[70] =
      "<script src=\"/welcome.js?username=#{ username }\" nonce=%s>";
  char html_page[500];

  printf("Username: ");
  printf(username);
  printf("\n");

  if (*status) {
    *status = -1; // defaults to error
  }

  sprintf(html_page, html_page_template, nonce);

  return html_page;
}
