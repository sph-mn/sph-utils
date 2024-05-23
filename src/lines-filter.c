#ifndef lines_filter_config
#define page_size 4096
#define max_thread_count 16
#define lines_filter_config
#endif

#ifndef read_all_paths_config
#define path_initial_length 400
#define paths_initial_count 10000
#define paths_initial_data_size paths_initial_count * path_initial_length
#define data_growth_factor 3
#define read_all_paths_config
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
#include "lib/case_table.c"
#include "lib/read_all_paths.c"

#define display_error(format, ...) fprintf(stderr, "error: %s:%d " format "\n", __func__, __LINE__, __VA_ARGS__)
#define errno_error do{display_error("%s", strerror(errno));exit(1);}while(0)

#define options_flag_negate 1
#define options_flag_or 2
#define options_flag_files 4
#define options_flag_null_delimiter 8

const uint8_t cli_help_text[] = "arguments: [options] pattern ...\n"
  "description\n"
  "  read from standard input and filter lines by given strings.\n"
  "options\n"
  "  -f read filesystem paths from standard input and read file content, prefixing the file name to matching lines\n"
  "  -h show this help text\n"
  "  -n negate\n"
  "  -o matching lines must contain only one of the patterns\n"
  "  -v display the current version number\n";

const uint8_t invalid_character_text[] = "invalid character. only characters in the ascii range 20 to 126 are accepted\n";
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

static uint8_t* memmem64(const uint8_t* data, size_t size, const uint8_t* pattern)
{
  // size must be >= 8. both data and pattern must be aligned for 64 bit access.
  uint64_t a = *(uint64_t*)data;
  uint64_t b = *(const uint64_t*)pattern;
  a += 8;
  size -= 8;
  for (; size; size -= 1, a = (a << 8) | *data++) if (b == a) return (uint8_t*)data - 8;
  return b == a ? (uint8_t*)data - 8 : 0;
}

/*
void read_files(uint8_t* paths_data, uint32_t* paths_indexes, uint32_t paths_indexes_used) {
  for (uint32_t i = 0; i < paths_indexes_used; i += 1) {
  }
}
*/

int main(int argc, char** argv) {
  uint8_t patterns_count;
  uint8_t** patterns;
  uint8_t options = cli(argc, argv, &patterns_count, &patterns);
  if (options & options_flag_files) {
    uint8_t* paths_data;
    uint32_t* paths_indexes;
    uint32_t paths_indexes_used;
    ssize_t read_size;
    read_all_paths(options & options_flag_null_delimiter ? 0 : '\n', &paths_data, &paths_indexes, &paths_indexes_used);
    for (uint32_t i = 0; i < paths_indexes_used; i += 1) {
      int file = open(paths_data + paths_indexes[i]);
      read_size = read(0, 8 * page_size);
      for (uint16_t j = 0; j < read_size && !buffer[j]; i += 1);

      write(1, paths_data + paths_index, paths_indexes[i] - paths_index);
    }
  }
  else {

  }
  return 0;
}
