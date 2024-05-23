#include <unistd.h>  // ssize_t, read
#include <inttypes.h>
#include <string.h>  // strlen, strerror
#include <errno.h>
#include "lib/tmalloc.c"
#include "lib/read_paths.c"

#define buffer_size 8192

    uint8_t* paths_data;
    uint32_t* paths_indexes;
    uint32_t paths_indexes_used;
    ssize_t read_size;
    read_paths(options & options_flag_null_delimiter ? 0 : '\n', &paths_data, &paths_indexes, &paths_indexes_used);

// overlap line buffer

int main(int argc, char** argv) {
  // read newline separated file paths from standard input and display the total file size in bits.
  ssize_t read_size;
  uint8_t buffer[buffer_size];
  int file;
  off_t size;
  size_t total;
  uint8_t digits;
  uint8_t* total_string;
  read_size = read(0, buffer, buffer_size);
  if (0 < read_size) {
    for (size_t i = 0; i < read_size && '\n' != buffer[i]; i += 1) {
      if  continue;
      if ('\n' == buffer[i]) {
        buffer[i] = 0;
        file = open(buffer[i], O_RDONLY);
        if (-1 == file) {
          perror("open");
          continue;
        }
        off_t size = lseek(file, 0, SEEK_END);
        if (-1 == size) {
          perror("lseek");
          close(file);
          continue;
        }
        total += size;
      }
    }
  }
  if (errno) {
    const uint8_t* message = strerror(errno);
    write(1, message, strlen(message));
    return 1;
  }
  digits = u32_digits(total);
  u32_string(total, digits, total_string);
  write(1, total_string, digits + 1);
  return 0;
}
