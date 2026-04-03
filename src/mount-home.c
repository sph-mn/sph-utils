#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
char arguments_text[] = "arguments: [-s] [-l] [-o mount_options] source ...\n";
char *find_basename(char *path) {
  char *slash;
  slash = strrchr(path, '/');
  if (slash) return slash + 1;
  return path;
}
void make_directory_path(char *path) {
  char buffer[PATH_MAX];
  char *character;
  stpcpy(buffer, path);
  character = buffer + 1;
  while (*character) {
    if (*character == '/') {
      *character = 0;
      mkdir(buffer, 0777);
      *character = '/';
    }
    character += 1;
  }
  mkdir(buffer, 0777);
}
uint8_t path_is_mountpoint(char *path) {
  struct stat path_status;
  struct stat parent_status;
  char parent_path[PATH_MAX];
  char *end;
  if (lstat(path, &path_status)) return 0;
  end = stpcpy(parent_path, path);
  stpcpy(end, "/..");
  stat(parent_path, &parent_status);
  if (path_status.st_dev != parent_status.st_dev) return 1;
  if (path_status.st_ino == parent_status.st_ino) return 1;
  return 0;
}
int run_command(char **arguments) {
  pid_t child_process;
  int status;
  child_process = fork();
  if (!child_process) {
    execvp(arguments[0], arguments);
    _exit(1);
  }
  waitpid(child_process, &status, 0);
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return 1;
}
int main(int argument_count, char **argument_values) {
  int argument_index;
  int mount_argument_count;
  uint8_t option_sudo;
  uint8_t option_label;
  char *option_text;
  char *home_path;
  char *source;
  char *target_name;
  char *target_path_end;
  char *mount_arguments[8];
  char mount_dir[PATH_MAX];
  char target_path[PATH_MAX];
  argument_index = 1;
  option_sudo = 0;
  option_label = 0;
  option_text = 0;
  while (argument_index < argument_count) {
    if (!strcmp(argument_values[argument_index], "-s")) {
      option_sudo = 1;
    } else if (!strcmp(argument_values[argument_index], "-l")) {
      option_label = 1;
    } else if (!strcmp(argument_values[argument_index], "-o")) {
      argument_index += 1;
      option_text = argument_values[argument_index];
    } else if (!strcmp(argument_values[argument_index], "--")) {
      argument_index += 1;
      break;
    } else {
      break;
    }
    argument_index += 1;
  }
  home_path = getenv("HOME");
  stpcpy(stpcpy(mount_dir, home_path), "/mnt");
  if (argument_index >= argument_count) {
    write(1, arguments_text, sizeof(arguments_text) - 1);
    dprintf(1, "mount_dir: %s\n", mount_dir);
    return 0;
  }
  make_directory_path(mount_dir);
  while (argument_index < argument_count) {
    source = argument_values[argument_index];
    if (option_label) {
      target_name = source;
    } else {
      target_name = find_basename(source);
    }
    target_path_end = stpcpy(target_path, mount_dir);
    target_path_end = stpcpy(target_path_end, "/");
    stpcpy(target_path_end, target_name);
    if (path_is_mountpoint(target_path)) {
      dprintf(1, "%s already mounted\n", target_path);
      argument_index += 1;
      continue;
    }
    make_directory_path(target_path);
    mount_argument_count = 0;
    if (option_sudo) {
      mount_arguments[mount_argument_count] = "sudo";
      mount_argument_count += 1;
    }
    mount_arguments[mount_argument_count] = "mount";
    mount_argument_count += 1;
    if (option_label) {
      mount_arguments[mount_argument_count] = "-L";
      mount_argument_count += 1;
      mount_arguments[mount_argument_count] = source;
      mount_argument_count += 1;
    } else {
      mount_arguments[mount_argument_count] = source;
      mount_argument_count += 1;
    }
    if (option_text) {
      mount_arguments[mount_argument_count] = "-o";
      mount_argument_count += 1;
      mount_arguments[mount_argument_count] = option_text;
      mount_argument_count += 1;
    }
    mount_arguments[mount_argument_count] = target_path;
    mount_argument_count += 1;
    mount_arguments[mount_argument_count] = 0;
    run_command(mount_arguments);
    argument_index += 1;
  }
  return 0;
}
