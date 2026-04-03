#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define mode_and 0
#define mode_or 1
#define maximum_line_length 8192
char arguments_text[] =
  "[options] string ...\n"
  "  -a  matching lines must contain all strings. the default\n"
  "  -n  negate\n"
  "  -o  matching lines must contain at least one of the strings\n";
int main(int argument_count, char **argument_values) {
  int argument_index;
  uint16_t pattern_index;
  uint16_t line_length;
  uint16_t pattern_count;
  uint8_t ignore_case;
  uint8_t matched;
  uint8_t mode;
  uint8_t negate;
  char line[maximum_line_length];
  char lowered_line[maximum_line_length];
  char *match_line;
  char **patterns;
  char *pattern;
  char character;
  argument_index = 1;
  ignore_case = 1;
  mode = mode_and;
  negate = 0;
  while (argument_index < argument_count) {
    if (!strcmp(argument_values[argument_index], "-a")) mode = mode_and;
    else if (!strcmp(argument_values[argument_index], "-o")) mode = mode_or;
    else if (!strcmp(argument_values[argument_index], "-n")) negate = 1;
    else break;
    argument_index += 1;
  }
  pattern_count = argument_count - argument_index;
  if (!pattern_count) {
    write(2, arguments_text, sizeof(arguments_text) - 1);
    return 1;
  }
  patterns = argument_values + argument_index;
  for (pattern_index = 0; pattern_index < pattern_count; pattern_index += 1) {
    pattern = patterns[pattern_index];
    while (*pattern) {
      character = *pattern;
      if (character >= 'A' && character <= 'Z') {
        ignore_case = 0;
        break;
      }
      pattern += 1;
    }
    if (!ignore_case) break;
  }
  while (fgets(line, sizeof(line), stdin)) {
    line_length = strlen(line);
    if (ignore_case) {
      for (pattern_index = 0; pattern_index <= line_length; pattern_index += 1) {
        character = line[pattern_index];
        if (character >= 'A' && character <= 'Z') lowered_line[pattern_index] = character | 0x20;
        else lowered_line[pattern_index] = character;
      }
      match_line = lowered_line;
    } else match_line = line;
    if (mode == mode_and) {
      matched = 1;
      for (pattern_index = 0; pattern_index < pattern_count; pattern_index += 1)
        if (!strstr(match_line, patterns[pattern_index])) {
          matched = 0;
          break;
        }
    } else {
      matched = 0;
      for (pattern_index = 0; pattern_index < pattern_count; pattern_index += 1)
        if (strstr(match_line, patterns[pattern_index])) {
          matched = 1;
          break;
        }
    }
    if (negate) matched = !matched;
    if (matched) write(1, line, line_length);
  }
  return 0;
}
