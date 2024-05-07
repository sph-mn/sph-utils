#ifndef lines_filter_config
#define page_size 4096
#define path_initial_length 400
#define paths_initial_count 10000
#define paths_initial_data_size paths_initial_count * path_initial_length
#define growth_factor 3
#define lines_filter_config
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <sys/mman.h>

#define print(fd, stack_string_variable) write(fd, stack_string_variable, sizeof(stack_string_variable) - 1)
#define tmalloc(size) mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
#define trealloc(pointer, old_size, new_size) mremap(pointer, old_size, new_size, MREMAP_MAYMOVE)
#define tmalloc_is_failure(pointer) MAP_FAILED == pointer
#define display_error(format, ...) fprintf(stderr, "error: %s:%d " format "\n", __func__, __LINE__, __VA_ARGS__)
#define errno_error do{display_error("%s", strerror(errno));exit(1);}while(0)

#define options_flag_negate 1
#define options_flag_or 2
#define options_flag_files 4
#define options_flag_null_delimiter 8

const uint8_t case_table[] = {
  ['A'] = 'a', ['B'] = 'b', ['C'] = 'c', ['D'] = 'd', ['E'] = 'e', ['F'] = 'f',
  ['G'] = 'g', ['H'] = 'h', ['I'] = 'i', ['J'] = 'j', ['K'] = 'k', ['L'] = 'l',
  ['M'] = 'm', ['N'] = 'n', ['O'] = 'o', ['P'] = 'p', ['Q'] = 'q', ['R'] = 'r',
  ['S'] = 's', ['T'] = 't', ['U'] = 'u', ['V'] = 'v', ['W'] = 'w', ['X'] = 'x',
  ['Y'] = 'y', ['Z'] = 'z'
};

const uint8_t cli_help_text[] = "arguments: [options] pattern ...\n"
  "description\n"
  "  read from standard input and filter lines by given strings.\n"
  "  matches portions for long lines.\n"
  "options\n"
  "  -f read filesystem paths from standard input and filter file content, prefixing the file name to matching lines\n"
  "  -h show this help text\n"
  "  -s display source file name only. may be displayed multiple times"
  "  -n negate\n"
  "  -o matching lines must contain only one of the patterns\n"
  "  -v display the current version number\n";

const uint8_t cli_version_text[] = "v0.1\n";

uint8_t cli(int argc, char** argv, uint16_t* patterns_count, uint8_t*** patterns) {
  int opt;
  uint8_t options = 0;
  while (!(-1 == (opt = getopt(argc, argv, "fhnov")))) {
    if ('f' == opt) options |= options_flag_files;
    else if ('h' == opt) {
      print(1, cli_help_text);
      exit(0);
    }
    else if ('n' == opt) options |= options_flag_negate;
    else if ('o' == opt) options |= options_flag_or;
    else if ('v' == opt) {
      print(1, cli_version_text);
      exit(0);
    }
  }
  if (argc == optind) {
    print(1, cli_help_text);
    exit(2);
  }
  *patterns_count = argc - optind;
  *patterns = (uint8_t**)argv + optind;
  return options;
}

void read_paths(char delimiter, uint8_t** data, uint32_t** indexes, uint32_t* indexes_used) {
  // read delimiter-terminated paths from standard input.
  // all paths must end with the delimiter.
  // data will contain the full string read from standard input,
  // while paths contains indexes to the start of each path in data.
  // the delimiter will be replaced with 0 to make paths null-separated strings (needed for libc system calls).
  uint32_t data_used = 0;
  uint32_t data_size = paths_initial_data_size;
  uint32_t indexes_size = paths_initial_count;
  ssize_t read_size;
  *data = tmalloc(data_size);
  if (tmalloc_is_failure(*data)) errno_error;
  *indexes = tmalloc(indexes_size * sizeof(uint32_t));
  if (tmalloc_is_failure(*indexes)) errno_error;
  **indexes = 0;
  *indexes_used = 1;
  while (0 < read(0, *data + data_used, paths_initial_data_size / 2)) {
    if (data_size < data_used + read_size + paths_initial_data_size / 2) {
      *data = trealloc(*data, data_size, data_size * growth_factor);
      if (tmalloc_is_failure(*data)) errno_error;
      data_size *= growth_factor;
    }
    for (uint32_t i = 0; i < read_size; i += 1) {
      if (delimiter == (*data)[data_used + i]) {
        if (indexes_size < *indexes_used) {
          *indexes = trealloc(*indexes, indexes_size * sizeof(uint32_t), indexes_size * growth_factor * sizeof(uint32_t));
          if (tmalloc_is_failure(*indexes)) errno_error;
          indexes_size *= growth_factor;
        }
        (*data)[data_used + i] = 0;
        (*indexes)[*indexes_used] = data_used + i;
        *indexes_used += 1;
      }
    }
    data_used += read_size;
  }
  if (0 > read_size) errno_error;
  // correct for a trailing newline
  if (delimiter == (*data)[data_used]) *indexes_used -= 1;
}

/*
void read_files(uint8_t* paths_data, uint32_t* paths_indexes, uint32_t paths_indexes_used) {
  for (uint32_t i = 0; i < paths_indexes_used; i += 1) {
  }
}
*/

int main(int argc, char** argv) {
  uint16_t patterns_count;
  uint8_t** patterns;
  uint8_t options = cli(argc, argv, &patterns_count, &patterns);
  if (options & options_flag_files) {
    uint8_t* paths_data;
    uint32_t* paths_indexes;
    uint32_t paths_indexes_used;
    ssize_t read_size;
    read_paths(options & options_flag_null_delimiter ? 0 : '\n', &paths_data, &paths_indexes, &paths_indexes_used);
    for (uint32_t i = 0; i < paths_indexes_used; i += 1) {
      int file = open(paths_data + paths_indexes[i]);
      read_size = read(0, 8 * page_size);
      for (uint16_t j = 0; j < read_size && !buffer[j]; i += 1);

      write(1, paths_data + paths_index, paths_indexes[i] - paths_index);
    }
  }
  return 0;
}
