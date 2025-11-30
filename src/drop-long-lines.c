#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv) {
  ssize_t max_length;
  char *buffer;
  size_t buffer_size;
  ssize_t bytes_read;

  max_length = 300;
  if (argc > 1 && argv[1][0] != '\0') {
    max_length = (ssize_t)strtoul(argv[1], 0, 10);
  }

  buffer = 0;
  buffer_size = 0;

  while (1) {
    bytes_read = getline(&buffer, &buffer_size, stdin);
    if (bytes_read < 0) break;
    if (bytes_read <= max_length) {
      fwrite(buffer, 1, (size_t)bytes_read, stdout);
    }
  }

  free(buffer);
  return 0;
}
