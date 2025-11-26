#include <unistd.h>  // write
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>  // mkdir
#include <string.h>  // memcpy
#include <stdio.h>  // rename

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "lib/ensure_directory_structure.c"
#include "lib/print_errno.c"

int main(int argc, char** argv) {
  char help_text[] = "arguments: target path ...\n";
  char duplicate_text[] = "file exists\n";
  char target[PATH_MAX];
  uint16_t target_len, source_len, basename_index;
  if (3 > argc) {
    write(1, help_text, sizeof(help_text));
    return 2;
  }
  // prepare target directory
  target_len = strlen(argv[1]);
  memcpy(target, argv[1], target_len);
  target[target_len] = 0;
  if (ensure_directory_structure(target)) {
    print_errno();
    return 1;
  }
  // move files
  target[target_len] = '/';
  target_len += 1;
  for (int i = 2; i < argc; i += 1) {
    source_len = strlen(argv[i]);
    // skip trailing slashes
    while (source_len && '/' == argv[i][source_len - 1]) source_len -= 1;
    // find basename start
    basename_index = source_len;
    while (basename_index && '/' != argv[i][basename_index - 1]) basename_index -= 1;
    source_len -= basename_index;
    // add basename to target
    memcpy(target + target_len, argv[i] + basename_index, source_len);
    target[target_len + source_len] = 0;
    // rename
    if (access(target, F_OK)) rename(argv[i], target);
    else write(2, duplicate_text, sizeof(duplicate_text));
  }
  return 0;
}
