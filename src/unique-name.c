#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#define max_suffix_length 255

int main(int argc, char** argv) {
  size_t counter = 1;
  size_t path1_len;
  char suffix_length = 0;
  char* path1;
  char* path2;
  if (2 != argc) {
    printf("arguments: path\n");
    return 1;
  }
  path1 = argv[1];
  if (access(path1, F_OK)) {
    write(1, path1, strlen(argv[1]));
    write(1, "\n", 1);
    return 0;
  }
  path1_len = strlen(path1);
  path2 = malloc(path1_len + max_suffix_length);
  memcpy(path2, path1, path1_len);
  if (!path2) {
    fprintf(stderr, "could not allocate memory\n");
    return 1;
  }
  do {
    suffix_length = snprintf(path2 + path1_len, max_suffix_length, ".%ju", counter);
    if (-1 == suffix_length || SIZE_MAX == counter) {
      return 1;
    }
    counter += 1;
  } while (!access(path2, F_OK));
  write(1, path2, path1_len + max_suffix_length);
  write(1, "\n", 1);
  return 0;
}
