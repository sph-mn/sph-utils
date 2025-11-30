#include <stdio.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int resolve_duplicate_filename(char* path, size_t path_length) {
  size_t counter = 1;
  int8_t suffix_length = 0;
  do {
    suffix_length = snprintf(path + path_length, PATH_MAX - path_length, ".%ju", counter);
    if (-1 == suffix_length) {
      return 1;
    }
    counter += 1;
  } while (!access(path, F_OK));
  return 0;
}
