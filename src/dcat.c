#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>
#include "lib/print_errno.c"

#define basename_length 256
#define max_path_length 16 * basename_length
#define entries_size 16 * basename_length

#define string_chars_equal2(a, c1, c2) (a[0] == c1 && (!a[1] || (a[1] == c2 && !a[2])))

struct linux_dirent64 {
  ino64_t        d_ino;
  off64_t        d_off;
  unsigned short d_reclen;
  unsigned char  d_type;
  char           d_name[];
};

uint8_t path[max_path_length];
uint8_t long_path_text[] = "path too long\n";

int enter(uint8_t* path, uint16_t path_len, uint8_t depth) {
  int fd;
  ssize_t read_size;
  struct linux_dirent64* entry;
  uint8_t d_name_len;
  uint8_t entries[entries_size];
  path[path_len] = 0;
  fd = open(path, O_RDONLY | O_DIRECTORY);
  if (0 > fd) {
    if (errno) print_errno();
    return 1;
  }
  path[path_len] = '/';
  path_len += 1;
  while (0 < (read_size = syscall(SYS_getdents64, fd, entries, entries_size))) {
    int i = 0;
    do {
      entry = (struct linux_dirent64*)(entries + i);
      // skip standard directory references
      if (!string_chars_equal2(entry->d_name, '.', '.')) {
        d_name_len = strlen(entry->d_name);
        if (max_path_length <= path_len + d_name_len) {
          i += entry->d_reclen;
          write(2, long_path_text, strlen(long_path_text));
          continue;
        }
        memcpy(path + path_len, entry->d_name, d_name_len);
        path[path_len + d_name_len] = '\n';
        write(1, path, path_len + d_name_len + 1);
        if (depth && DT_DIR == entry->d_type) {
          enter(path, path_len + d_name_len, depth - 1);
        }
      }
      i += entry->d_reclen;
    } while (i < read_size);
  }
  close(fd);
  if (0 > read_size) {
    print_errno();
    return 1;
  }
  return 0;
}

int main(int argc, char** argv) {
  if (1 < argc) {
    uint8_t i = 1;
    uint8_t depth = 0;
    uint16_t path_len;
    int exit_code = 0;
    if (string_chars_equal2(argv[i], '-', 'r')) {
      depth = 255;
      i += 1;
    }
    if (argc == i) {
      path[0] = '.';
      return enter(path, 1, depth);
    }
    else {
      for (; i < argc; i += 1) {
        path_len = strlen(argv[i]);
        memcpy(path, argv[i], path_len);
        exit_code |= enter(path, path_len, depth);
      }
      return exit_code;
    }
  }
  else {
    path[0] = '.';
    return enter(path, 1, 0);
  }
}
