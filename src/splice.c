
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define path_length_max 4096
char help_text[] =
  "arguments: directory ...\n"
  "description\n"
  "  merge the files of the given directories with their parent directories.\n"
  "  files with duplicate names are kept by renaming.\n"
  "options\n"
  "  -1  splice if directory contains at most one file\n"
  "  -h  display this help text\n"
  "  -v  display the current version number\n";
char version_text[] = "v0.1\n";
char missing_directory_text[] = "missing argument: directory\n";
char inaccessible_directory_text[] = "directory not accessible. ";
char path_too_long_text[] = "path too long. ";
char duplicate_rename_failed_text[] = "target already exists and renaming source failed. ";
int resolve_duplicate_filename(char *path, size_t path_length) {
  size_t counter;
  int suffix_length;
  counter = 1;
  while (!access(path, F_OK)) {
    suffix_length = snprintf(path + path_length, path_length_max + 1 - path_length, ".%ju", counter);
    if (suffix_length < 0) return 1;
    counter += 1;
  }
  return 0;
}
void print_errno(void) {
  char *message;
  message = strerror(errno);
  write(2, message, strlen(message));
  write(2, "\n", 1);
}
int main(int argument_count, char **argument_values) {
  int argument_index;
  uint8_t option_single;
  DIR *directory;
  struct dirent *entry;
  char **directories;
  size_t directory_count;
  size_t directory_index;
  size_t directory_length;
  size_t entry_length;
  char source_path[path_length_max + 1];
  char target_path[path_length_max + 1];
  char *path_end;
  argument_index = 1;
  option_single = 0;
  while (argument_index < argument_count) {
    if (!strcmp(argument_values[argument_index], "-1")) option_single = 1;
    else if (!strcmp(argument_values[argument_index], "-h")) {
      write(1, help_text, sizeof(help_text) - 1);
      return 0;
    } else if (!strcmp(argument_values[argument_index], "-v")) {
      write(1, version_text, sizeof(version_text) - 1);
      return 0;
    } else break;
    argument_index += 1;
  }
  if (argument_index == argument_count) {
    write(2, missing_directory_text, sizeof(missing_directory_text) - 1);
    return 1;
  }
  directory_count = argument_count - argument_index;
  directories = argument_values + argument_index;
  for (directory_index = 0; directory_index < directory_count; directory_index += 1) {
    directory = opendir(directories[directory_index]);
    if (!directory) {
      write(2, inaccessible_directory_text, sizeof(inaccessible_directory_text) - 1);
      write(2, directories[directory_index], strlen(directories[directory_index]));
      write(2, "\n", 1);
      continue;
    }
    if (option_single) {
      if (readdir(directory) && readdir(directory) && readdir(directory) && readdir(directory)) {
        closedir(directory);
        continue;
      }
      rewinddir(directory);
    }
    directory_length = strlen(directories[directory_index]);
    if (path_length_max < directory_length + 5) {
      write(2, path_too_long_text, sizeof(path_too_long_text) - 1);
      write(2, directories[directory_index], directory_length);
      write(2, "\n", 1);
      closedir(directory);
      continue;
    }
    path_end = stpcpy(source_path, directories[directory_index]);
    *path_end = '/';
    path_end += 1;
    *path_end = 0;
    path_end = stpcpy(target_path, directories[directory_index]);
    *path_end = '/';
    path_end += 1;
    *path_end = '.';
    path_end += 1;
    *path_end = '.';
    path_end += 1;
    *path_end = '/';
    path_end += 1;
    *path_end = 0;
    while ((entry = readdir(directory))) {
      if (!strcmp(entry->d_name, ".")) continue;
      if (!strcmp(entry->d_name, "..")) continue;
      entry_length = strlen(entry->d_name);
      if (path_length_max < directory_length + 3 + entry_length + 1) {
        write(2, path_too_long_text, sizeof(path_too_long_text) - 1);
        write(2, directories[directory_index], directory_length);
        write(2, "\n", 1);
        continue;
      }
      stpcpy(source_path + directory_length + 1, entry->d_name);
      stpcpy(target_path + directory_length + 4, entry->d_name);
      if (!access(target_path, F_OK)) {
        if (resolve_duplicate_filename(target_path, directory_length + 3 + entry_length + 1)) {
          write(2, duplicate_rename_failed_text, sizeof(duplicate_rename_failed_text) - 1);
          write(2, target_path, strlen(target_path));
          write(2, "\n", 1);
          continue;
        }
      }
      if (rename(source_path, target_path)) print_errno();
    }
    closedir(directory);
    rmdir(directories[directory_index]);
  }
  return 0;
}
