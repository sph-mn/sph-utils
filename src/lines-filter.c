#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#define mode_and 0
#define mode_or 1
#define maximum_line_length 8192

uint8_t no_patterns_text[] = "no search patterns specified.\n";
uint8_t usage_text[] = "lines-filter [options] string ...\n"
  "  -a  matching lines must contain all strings. the default\n"
  "  -n  negate\n"
  "  -o  matching lines must contain at least one of the strings\n";

int main(int argc, char** argv) {
  int8_t opt;
  uint16_t i;
  uint16_t line_length;
  uint16_t num_patterns = 0;
  uint8_t case_insensitive = 1;
  uint8_t line[maximum_line_length];
  uint8_t matched;
  uint8_t match_line_data[maximum_line_length];
  uint8_t* match_line;
  uint8_t mode = mode_and;
  uint8_t negate = 0;
  uint8_t** patterns = 0;

  // parse options
  while ((opt = getopt(argc, argv, "aon")) != -1) {
    if ('a' == opt) mode = mode_and;
    else if ('o' == opt) mode = mode_or;
    else if ('n' == opt) negate = 1;
    else {
      write(2, usage_text, strlen(usage_text));
      return 1;
    }
  }

  // parse search strings
  num_patterns = argc - optind;
  if (num_patterns > 0) patterns = (uint8_t**)(argv + optind);
  else {
    write(2, no_patterns_text, strlen(no_patterns_text));
    write(2, usage_text, strlen(usage_text));
    return 1;
  }

  // check for uppercase letters in patterns
  for (i = 0; i < num_patterns; i += 1) {
    uint8_t* p = patterns[i];
    while (*p) {
      if (*p >= 'A' && *p <= 'Z') {
        case_insensitive = 0;
        break;
      }
      p += 1;
    }
    if (!case_insensitive) break;
  }

  // read and match lines
  while (fgets(line, sizeof(line), stdin) != 0) {
    line_length = strlen(line);
    if (case_insensitive) {
      for (i = 0; i <= line_length; i += 1) {
        match_line_data[i] = (line[i] >= 'A' && line[i] <= 'Z') ? line[i] | 0x20 : line[i];
      }
      match_line = match_line_data;
    }
    else match_line = line;
    matched = 0;
    if (mode == mode_and) {
      matched = 1;
      for (i = 0; i < num_patterns; i += 1) {
        if (strstr(match_line, patterns[i]) == 0) {
          matched = 0;
          break;
        }
      }
    } else if (mode == mode_or) {
      for (i = 0; i < num_patterns; i += 1) {
        if (strstr(match_line, patterns[i]) != 0) {
          matched = 1;
          break;
        }
      }
    }
    if (negate) matched = !matched;
    if (matched) write(1, line, line_length);
  }

  return 0;
}
