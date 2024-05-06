#include <unistd.h>  // ssize_t, read
#include <inttypes.h>
#include <string.h>  // strlen, strerror
#include <errno.h>

#define buffer_size 16384

inline uint8_t u32_digits(uint32_t n) {
  if (n < 10) return 1;
  if (n < 100) return 2;
  if (n < 1000) return 3;
  if (n < 10000) return 4;
  if (n < 100000) return 5;
  if (n < 1000000) return 6;
  if (n < 10000000) return 7;
  if (n < 100000000) return 8;
  if (n < 1000000000) return 9;
  return 10;
}

void u32_string(uint32_t a, uint8_t digits, uint8_t* b) {
  // countlut number to string conversion from https://github.com/fmtlib/format-benchmark/tree/master/src/itoa-benchmark
  static const char table[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
    '1', '0', '1', '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
    '2', '0', '2', '1', '2', '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
    '4', '0', '4', '1', '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
    '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
    '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
    '8', '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
  };
  b += digits;
  *b = '\n';
  while (a >= 100) {
    const uint16_t i = (a % 100) << 1;
    a /= 100;
    *--b = table[i + 1];
    *--b = table[i];
  }
  if (a < 10) *--b = a + '0';
  else {
    const uint16_t i = a << 1;
    *--b = table[i + 1];
    *--b = table[i];
  }
}

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
