#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <inttypes.h>
#include <errno.h>
#include "lib/resolve_duplicate_filename.c"
#include "lib/print_errno.c"

#define print(fd, var_name, string_literal)      \
  uint8_t var_name[] = string_literal; \
  write(fd, var_name, sizeof(var_name) - 1);
#define path_len_max 4096
#define path_len_t uint16_t

void cli(int argc, char** argv, size_t* directories_count, uint8_t*** directories, uint8_t* single) {
  int opt;
  *single = 0;
  while (!(-1 == (opt = getopt(argc, argv, "1hv")))) {
    if ('1' == opt) {
      *single = 1;
    }
    else if ('h' == opt) {
      print(1, help_text, "arguments: directory ...\n"
        "description\n"
        "  merge the files of the given directories with their parent directories.\n"
        "  files with duplicate names are kept by renaming.\n"
        "options\n"
        "  -1  splice if directory contains at most one file\n"
        "  -h  display this help text\n"
        "  -v  display the current version number\n");
      exit(0);
    }
    else if ('v' == opt) {
      print(1, version_text, "v0.1\n");
      exit(0);
    }
  }
  if (argc == optind) {
    print(2, missing_directory_text, "missing argument: directory\n");
    exit(1);
  }
  *directories_count = argc - optind;
  *directories = (uint8_t**)argv + optind;
}

int main(int argc, char** argv) {
  DIR* dir;
  uint8_t path1[path_len_max + 1];
  uint8_t path2[path_len_max + 1];
  uint8_t single;
  uint8_t* path;
  uint8_t** directories;
  size_t directories_count;
  struct dirent* entry;
  path_len_t directory_strlen;
  path_len_t entry_strlen;
  cli(argc, argv, &directories_count, &directories, &single);
  for (size_t i = 0; i < directories_count; i += 1) {
    dir = opendir(directories[i]);
    if (!dir) {
      fprintf(stderr, "directory not accessible. %s\n", directories[i]);
      continue;
    }
    if (single) {
      if (readdir(dir) && readdir(dir) && readdir(dir) && readdir(dir)) {
        continue;
      }
      rewinddir(dir);
    }
    // limit path length
    directory_strlen = strlen(directories[i]);
    if (path_len_max < directory_strlen + 5) {
      fprintf(stderr, "path too long. %s\n", directories[i]);
    }
    memcpy(path1, directories[i], directory_strlen);
    path1[directory_strlen] = '/';
    memcpy(path2, directories[i], directory_strlen);
    path2[directory_strlen] = '/';
    while (entry = readdir(dir)) {
      if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)) {
        continue;
      }
      // limit path length
      entry_strlen = strlen(entry->d_name);
      if (path_len_max < directory_strlen + 3 + entry_strlen + 1) {
        fprintf(stderr, "path too long. %s\n", directories[i]);
      }
      // source path
      path = path1 + directory_strlen + 1;
      memcpy(path, entry->d_name, entry_strlen);
      path += entry_strlen;
      *path = 0;
      // target path
      path = path2 + directory_strlen + 1;
      *path = '.';
      path += 1;
      *path = '.';
      path += 1;
      *path = '/';
      path += 1;
      memcpy(path, entry->d_name, entry_strlen);
      path += entry_strlen;
      *path = 0;
      // resolve duplicates
      if (!access(path2, F_OK)) {
        if (resolve_duplicate_filename(path2, directory_strlen + 3 + entry_strlen + 1)) {
          fprintf(stderr, "target already exists and renaming source failed. %s\n", path2);
          continue;
        }
      }
      // move
      if (rename(path1, path2)) {
        print_errno();
      }
    }
    closedir(dir);
    rmdir(directories[i]);
  }
  return 0;
}
