#include <unistd.h>  // ssize_t, read
#include <inttypes.h>
#include <string.h>  // strlen, strerror
#include <errno.h>
#include "lib.c"

#define buffer_size 16384

int main(int argc, char** argv) {
  // read from standard input and write the character counts for each line to standard output.
  uint32_t start = 0;
  uint32_t length = 0;
  ssize_t read_size;
  uint8_t buffer[buffer_size];
  uint8_t digits;
  uint8_t length_string[11];
  while (0 < (read_size = read(0, buffer, buffer_size))) {
    for (size_t i = 0; i < read_size; i += 1) {
      if ('\n' == buffer[i]) {
        length += i - start;
        digits = u32_digits(length);
        u32_string(length, digits, length_string);
        write(1, length_string, digits + 1);
        length = 0;
        start = i + 1;
      }
    }
    length += start;
  }
  if (errno) {
    const uint8_t* message = strerror(errno);
    write(1, message, strlen(message));
    return 1;
  }
  return 0;
}
