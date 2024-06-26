#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <libgen.h>  // dirname
#include <stdlib.h>  // realpath
#include <string.h>
#include <sys/stat.h>  // mkdir
#include <unistd.h>  // write, access

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "lib/print_errno.c"
#include "lib/ensure_directory_structure.c"
#include "lib/resolve_duplicate_filename.c"

#include <stdio.h>

#define string_chars_equal2(a, c1, c2) (a[0] == c1 && (!a[1] || (a[1] == c2 && !a[2])))
#define option_add 1
#define option_subtract 2

uint16_t u16_from_string(uint8_t* a, uint8_t len) {
  // len must be >= 1
  uint16_t b;
  uint16_t factor = 10;
  len -= 1;
  b = a[len] - 48;
  if (len) {
    while (len--) {
      b += (a[len] - 48) * factor;
      factor *= 10;
    }
  }
  return b;
}

int cli(int argc, char** argv, uint16_t* new_rating, uint8_t* options) {
  uint8_t help_text[] = "arguments: [-m] rating path ...\n";
  uint16_t len;
  if (3 > argc) {
    write(2, help_text, strlen(help_text));
    exit(1);
  }
  int i = 1;
  // integer overflows for invalid rating values
  if (string_chars_equal2(argv[i], '-', 'm')) {
    i += 1;
    len = strlen(argv[i]);
    if ('-' == argv[i][0]) {
      *options = option_subtract;
      *new_rating = u16_from_string(argv[i] + 1, len - 1);
    }
    else {
      *options = option_add;
      *new_rating = u16_from_string(argv[i], len);
    }
  }
  else {
    *options = 0;
    *new_rating = u16_from_string(argv[i], len);
  }
  return i + 1;
}

void get_modified_path() {
  uint16_t rating_string_len = 0;
  uint16_t rating_string_start = 0;
  uint16_t len = strlen(old_path);
  while (len--) {
    if (rating_string_start && 48 <= old_path[len] && 57 >= old_path[len]) continue;
    else if ('/' == old_path[len]) {
      if (rating_string_start) {
        rating_string_len = rating_string_start - len - 1;
        memcpy(rating_string, old_path + len + 1, rating_string_len);
        break;
      }
      else rating_string_start = len;
    }
    else rating_string_start = 0;
  }
  // todo: old_rating_position
  if (rating_string_len) {
    old_rating = u16_from_string(rating_string, rating_string_len);
    // todo: use string representation right here and save conversion
    if (option_add == options) {
      new_rating += old_rating;
    }
    else if (option_subtract == options) {
      new_rating = old_rating - new_rating;
    }
  }
  else {
    if (options) new_rating = 0;
  }
  rating_string[rating_string_len] = 0;
  printf("path %s\n", rating_string);
}

int main(int argc, char** argv) {
  uint8_t old_path[path_max];
  uint8_t new_path[path_max];
  uint8_t rating_string[4];
  uint64_t rating_string_start;
  uint8_t rating_string_len;
  uint16_t old_rating;
  uint16_t new_rating;
  uint16_t len;
  uint8_t options;
  uint8_t* a;
  for (int i = cli(argc, argv, &new_rating, &options); i < argc; i += 1) {
    // resolve symlinks and directory references
    if (!realpath(argv[i], old_path)) {
      print_errno();
      continue;
    }
    uint16_t rating_string_len = 0;
    uint16_t rating_string_start = 0;
    uint16_t len = strlen(old_path);
    while (len--) {
      if (rating_string_start && 48 <= old_path[len] && 57 >= old_path[len]) continue;
      else if ('/' == old_path[len]) {
        if (rating_string_start) {
          rating_string_len = rating_string_start - len - 1;
          memcpy(rating_string, old_path + len + 1, rating_string_len);
          break;
        }
        else rating_string_start = len;
      }
      else rating_string_start = 0;
    }


    get_new_rating(old_path);
    continue;
    // todo: copy prefix
    memcpy(new_path, old_path, len);
    // todo: append new rating
    // todo: copy suffix
    memcpy(new_path, old_path, len);
    if (file_exists(new_path)) {
      if (resolve_duplicate_filename(path2, directory_strlen + 3 + entry_strlen + 1)) {
        fprintf(stderr, "target already exists and renaming source failed. %s\n", path2);
        continue;
      }
    }
    else {
      // todo: find dirname and create it
      ensure_directory_structure(new_path);
    }
    if (rename(old_path, new_path)) {
      print_errno();
    }
  }
}
