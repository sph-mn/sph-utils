#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

#define basename_length 256
#define path_length 16 * basename_length
#define entries_size 16 * basename_length

struct linux_dirent64 {
  ino64_t        d_ino;
  off64_t        d_off;
  unsigned short d_reclen;
  unsigned char  d_type;
  char           d_name[];
};

uint8_t path[path_length];

int next(uint8_t* path, uint16_t path_len) {
  int fd;
  ssize_t read_size;
  struct linux_dirent64* entry;
  uint8_t d_name_length;
  uint8_t entries[entries_size];
  path[path_len] = 0;
  fd = open(path, O_RDONLY | O_DIRECTORY);
  if (0 > fd) return 1;
  path[path_len] = '/';
  path_len += 1;
  while (0 < (read_size = syscall(SYS_getdents64, fd, entries, entries_size))) {
    int i = 0;
    do {
      entry = (struct linux_dirent64*)(entries + i);
      if (!('.' == entry->d_name[0] && (!entry->d_name[1] || (entry->d_name[1] == '.' && !entry->d_name[2])))) {
        d_name_length = strlen(entry->d_name);
        memcpy(path + path_len, entry->d_name, d_name_length);
        path[path_len + d_name_length] = '\n';
        write(1, path, path_len + d_name_length + 1);
        if (DT_DIR == entry->d_type) {
          next(path, path_len + d_name_length);
        }
      }
      i += entry->d_reclen;
    } while (i < read_size);
  }
  if (0 > read_size) return 1;
  return 0;
}

// todo: return the correct exit code
int main(int argc, char** argv) {
  if (2 > argc) {
    path[0] = '.';
    next(path, 1);
  }
  else {
    uint16_t path_len;
    for (uint8_t i = 1; i < argc; i += 1) {
      path_len = strlen(argv[i]);
      memcpy(path, argv[i], path_len);
      next(path, path_len);
    }
  }
}
