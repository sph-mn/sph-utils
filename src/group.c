#include <unistd.h>  // write()
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "lib/resolve_duplicate_filename.c"
#include "lib/ensure_directory_structure.c"
#include "lib/print.c"
#include "lib/print_errno.c"

int main(int argc, char** argv) {
  uint8_t help_text[] = "arguments: target path ...\n";
  uint8_t duplicate_text[] = "target exists and renaming source failed. ";
  uint8_t target[PATH_MAX];
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
    // append source basename to target
    while (source_len && '/' == argv[i][source_len - 1]) source_len -= 1;
    basename_index = source_len;
    while (basename_index && '/' != argv[i][basename_index - 1]) basename_index -= 1;
    source_len -= basename_index;
    memcpy(target + target_len, argv[i] + basename_index, source_len);
    target[target_len + source_len] = 0;
    // resolve duplicates
    if (!access(target, F_OK) && resolve_duplicate_filename(target, target_len + source_len)) {
      print(2, duplicate_text);
      target[target_len + source_len] = '\n';
      write(2, target, target_len + source_len + 1);
      continue;
    }
    rename(argv[i], target);
  }
  return 0;
}
