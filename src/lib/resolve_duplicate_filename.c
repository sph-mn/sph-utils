#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int resolve_duplicate_filename(char *path, size_t path_length) {
  char *filename;
  char *extension;
  char suffix[3 * sizeof(size_t) + 2];
  size_t extension_length;
  size_t previous_length;
  size_t counter;
  int suffix_length;
  filename = strrchr(path, '/');
  if (filename) filename += 1;
  else filename = path;
  extension = strrchr(filename, '.');
  if (!extension || extension == filename || !extension[1]) extension = path + path_length;
  extension_length = strlen(extension);
  previous_length = 0;
  counter = 1;
  while (!access(path, F_OK)) {
    suffix_length = snprintf(suffix, sizeof(suffix), ".%zu", counter);
    if (suffix_length < 0 || path_length + suffix_length > PATH_MAX) return 1;
    memmove(extension + suffix_length, extension + previous_length, extension_length + 1);
    memcpy(extension, suffix, suffix_length);
    previous_length = suffix_length;
    counter += 1;
  }
  return 0;
}
