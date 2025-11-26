#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#define path_max 4096
void print_errno() {
  perror("");
}
void ensure_directory_structure(const char *path) {
  char tmp[path_max];
  snprintf(tmp, sizeof(tmp), "%s", path);
  size_t len = strlen(tmp);
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = '\0';
  }
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, S_IRWXU);
      *p = '/';
    }
  }
  mkdir(tmp, S_IRWXU);
}
int find_numeric_directory_in_path(const char *path, int *dir_start, int *dir_len) {
  int len = strlen(path);
  int end = len - 1;
  while (end >= 0 && path[end] == '/') {
    end--;
  }
  if (end < 0) {
    return 0;
  }
  while (end >= 0) {
    int start = end;
    while (start >= 0 && path[start] != '/') {
      start--;
    }
    int current_dir_start = start + 1;
    int current_dir_len = end - start;
    int all_digits = 1;
    for (int i = current_dir_start; i <= end; i++) {
      if (!isdigit((unsigned char)path[i])) {
        all_digits = 0;
        break;
      }
    }
    if (all_digits) {
      *dir_start = current_dir_start;
      *dir_len = current_dir_len;
      return 1;
    }
    end = start - 1;
    while (end >= 0 && path[end] == '/') {
      end--;
    }
  }
  return 0;
}
int cli(int argc, char **argv, int16_t *new_rating, uint8_t *options) {
  char help_text[] = "arguments: [-m] [+-]rating path ...\n";
  if (argc < 3) {
    write(2, help_text, strlen(help_text));
    exit(1);
  }
  int i = 1;
  *options = 0;
  if (strcmp(argv[i], "-m") == 0) {
    *options = 1;
    i += 1;
    if (i >= argc) {
      write(2, help_text, strlen(help_text));
      exit(1);
    }
  }
  char *rating_str = argv[i];
  int rating = atoi(rating_str);
  *new_rating = rating;
  i += 1;
  return i;
}
int non_symlink_abspath(const char *path, char *resolved) {
  char temp[path_max];
  if (path[0] == '/') {
    size_t path_len = strnlen(path, path_max - 1);
    memcpy(temp, path, path_len);
    temp[path_len] = '\0';
  } else {
    char cwd[path_max];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("getcwd");
      return -1;
    }
    size_t cwd_len = strnlen(cwd, path_max - 1);
    size_t path_len = strnlen(path, path_max - 1);
    if (cwd_len + 1 + path_len >= path_max) {
      if (path_max > cwd_len + 2) path_len = path_max - cwd_len - 2;
      else path_len = 0;
    }
    memcpy(temp, cwd, cwd_len);
    temp[cwd_len] = '/';
    memcpy(temp + cwd_len + 1, path, path_len);
    temp[cwd_len + 1 + path_len] = '\0';
  }
  char *stack[1024];
  int top = 0;
  char *token;
  token = strtok(temp, "/");
  while (token != NULL) {
    if (strcmp(token, ".") == 0) {
    } else if (strcmp(token, "..") == 0) {
      if (top > 0) top -= 1;
    } else {
      stack[top] = token;
      top = top + 1;
    }
    token = strtok(NULL, "/");
  }
  strcpy(resolved, "/");
  for (int i = 0; i < top; i++) {
    strcat(resolved, stack[i]);
    if (i < top - 1) {
      strcat(resolved, "/");
    }
  }
  return 0;
}
int main(int argc, char **argv) {
  int16_t new_rating;
  uint8_t options;
  char old_path[path_max];
  char new_path[path_max];
  int i = cli(argc, argv, &new_rating, &options);
  for (; i < argc; i++) {
    struct stat lst;
    if (lstat(argv[i], &lst) != 0) {
      perror("lstat");
      continue;
    }
    if (non_symlink_abspath(argv[i], old_path) != 0) continue;
    int dir_start, dir_len;
    int found = find_numeric_directory_in_path(old_path, &dir_start, &dir_len);
    if (found) {
      char numeric_dir[dir_len + 1];
      memcpy(numeric_dir, old_path + dir_start, dir_len);
      numeric_dir[dir_len] = '\0';
      int old_rating = atoi(numeric_dir);
      int modified_rating;
      if (options == 1) {
        modified_rating = old_rating + new_rating;
      }
      else {
        modified_rating = new_rating;
      }
      memcpy(new_path, old_path, dir_start);
      new_path[dir_start] = '\0';
      char modified_dir[16];
      sprintf(modified_dir, "%d", modified_rating);
      strcat(new_path, modified_dir);
      strcat(new_path, old_path + dir_start + dir_len);
    }
    else {
      char *slash = strrchr(old_path, '/');
      if (!slash) continue;
      size_t parent_len = (size_t)(slash - old_path);
      char parent[path_max];
      memcpy(parent, old_path, parent_len);
      parent[parent_len] = '\0';
      char rating_buf[16];
      sprintf(rating_buf, "%d", new_rating);
      size_t rating_len = strlen(rating_buf);
      size_t suffix_len = strlen(slash);
      if (parent_len + 1 + rating_len + suffix_len >= path_max) {
        if (path_max > parent_len + 1 + rating_len + 1) {
          suffix_len = path_max - parent_len - 1 - rating_len - 1;
        } else {
          suffix_len = 0;
        }
      }
      memcpy(new_path, parent, parent_len);
      new_path[parent_len] = '/';
      memcpy(new_path + parent_len + 1, rating_buf, rating_len);
      memcpy(new_path + parent_len + 1 + rating_len, slash, suffix_len);
      new_path[parent_len + 1 + rating_len + suffix_len] = '\0';
    }
    char *dir_end = strrchr(new_path, '/');
    if (dir_end) {
      *dir_end = '\0';
      ensure_directory_structure(new_path);
      *dir_end = '/';
    }
    struct stat st;
    if (stat(new_path, &st) == 0) {
      fprintf(stderr, "target file %s already exists\n", new_path);
      continue;
    }
    if (rename(old_path, new_path) != 0) {
      perror("rename");
      continue;
    }
    else {
      printf("moved %s to %s\n", old_path, new_path);
    }
  }
  return 0;
}
