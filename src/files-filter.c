#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define read_paths_display_error(format, ...) fprintf(stderr, "error: %s:%d " format "\n", __func__, __LINE__, __VA_ARGS__)
#define read_paths_memory_error() do {read_paths_display_error("%s", "memory allocation failed"); return 0;} while (0)

static const size_t paths_data_size_min = 4096;
typedef struct {
  uint8_t is_supported;
  uint8_t requires_value;
  uint8_t is_present;
  char* value;
} simple_option_parser_option_t;

#define simple_option_parser_declare(option_array_name) simple_option_parser_option_t option_array_name[256] = { 0 }
#define simple_option_parser_set_option( \
  option_array, option_character, requires_value_flag) \
  do { \
    uint8_t simple_option_parser_index; \
    simple_option_parser_index = (uint8_t)(unsigned char)(option_character); \
    (option_array)[simple_option_parser_index].is_supported = 1; \
    (option_array)[simple_option_parser_index].requires_value = \
      (requires_value_flag) ? 1 : 0; \
    (option_array)[simple_option_parser_index].is_present = 0; \
    (option_array)[simple_option_parser_index].value = 0; \
  } while (0)

uint8_t simple_option_parser(
  uint32_t argument_count,
  char** argument_values,
  simple_option_parser_option_t* options,
  uint32_t* rest_index)
{
  uint32_t index;
  uint32_t length;
  char* argument_value;
  uint8_t option_key;
  simple_option_parser_option_t* option;
  length = argument_count;
  index = 0;
  while (index < length) {
    argument_value = argument_values[index];
    if (
      argument_value[0] == '-' && argument_value[1] == '-' &&
      argument_value[2] == 0) {
      index += 1;
      break;
    }
    if (argument_value[0] != '-') break;
    if (argument_value[1] == 0) break;
    if (argument_value[2] != 0) break;
    option_key = (uint8_t)argument_value[1];
    option = options + option_key;
    if (option->is_supported == 0) break;
    option->is_present = 1;
    if (option->requires_value != 0) {
      if (index + 1 >= length) return 0;
      index += 1;
      option->value = argument_values[index];
      index += 1;
    } else {
      index += 1;
    }
  }
  *rest_index = index;
  return 1;
}

char* read_paths(char delimiter, char*** paths, size_t* paths_used)
{
  size_t data_size;
  size_t data_used;
  char* data;
  char* p;
  char* end;
  char* start;
  size_t paths_count;
  size_t paths_cap;
  ssize_t r;
  data_size = paths_data_size_min;
  data_used = 0;
  data = malloc(data_size);
  if (!data) read_paths_memory_error();
  for (;;) {
    if (data_used == data_size) {
      size_t new_data_size;
      char* new_data;
      new_data_size = data_size * 2;
      new_data = realloc(data, new_data_size);
      if (!new_data) {
        free(data);
        read_paths_memory_error();
      }
      data = new_data;
      data_size = new_data_size;
    }
    r = read(0, data + data_used, data_size - data_used);
    if (r <= 0) break;
    data_used = data_used + (size_t)r;
  }
  if (!data_used) {
    free(data);
    return 0;
  }
  p = data;
  end = data + data_used;
  start = p;
  paths_count = 0;
  while (p < end) {
    if (*p == delimiter) {
      if (p > start) paths_count = paths_count + 1;
      start = p + 1;
    }
    p += 1;
  }
  if (start < end) paths_count = paths_count + 1;
  paths_cap = paths_count;
  if (!paths_cap) paths_cap = 1;
  *paths = malloc(paths_cap * sizeof(char*));
  if (!*paths) {
    free(data);
    read_paths_memory_error();
  }
  *paths_used = 0;
  p = data;
  start = p;
  while (p < end) {
    if (*p == delimiter) {
      if (p > start) {
        *p = 0;
        (*paths)[*paths_used] = start;
        *paths_used = *paths_used + 1;
      }
      start = p + 1;
    }
    p += 1;
  }
  if (start < end) {
    (*paths)[*paths_used] = start;
    *paths_used = *paths_used + 1;
  }
  return data;
}

#define match_mode_all 0
#define match_mode_some 1
#define line_mode_line 0
#define line_mode_file 1
#define list_mode_paths_only 0
#define list_mode_paths_and_lines 1

typedef struct {
  char** paths;
  size_t paths_used;
  char** keywords;
  uint32_t keyword_count;
  uint8_t case_insensitive;
  uint8_t match_mode;
  uint8_t line_mode;
  uint8_t list_mode;
  uint8_t negate;
  size_t next_index;
  pthread_mutex_t index_mutex;
  pthread_mutex_t output_mutex;
} worker_context_t;

static uint8_t has_uppercase(const char* s)
{
  const unsigned char* p;
  p = (const unsigned char*)s;
  while (*p) {
    if (*p >= 'A' && *p <= 'Z') return 1;
    p += 1;
  }
  return 0;
}

static uint8_t keywords_case_insensitive(char** keywords, uint32_t keyword_count)
{
  uint32_t index;
  index = 0;
  while (index < keyword_count) {
    if (has_uppercase(keywords[index])) return 0;
    index = index + 1;
  }
  return 1;
}

static void lowercase_in_place(char* s)
{
  unsigned char* p;
  p = (unsigned char*)s;
  while (*p) {
    if (*p >= 'A' && *p <= 'Z') *p = (unsigned char)(*p | 0x20);
    p = p + 1;
  }
}

static uint8_t match_buffer(
  char* buffer,
  size_t length,
  char** keywords,
  uint32_t keyword_count,
  uint8_t match_mode)
{
  uint32_t i;
  uint8_t matched;
  buffer[length] = 0;
  matched = 0;
  if (match_mode == match_mode_all) {
    matched = 1;
    i = 0;
    while (i < keyword_count) {
      if (strstr(buffer, keywords[i]) == 0) {
        matched = 0;
        break;
      }
      i = i + 1;
    }
  } else {
    i = 0;
    while (i < keyword_count) {
      if (strstr(buffer, keywords[i]) != 0) {
        matched = 1;
        break;
      }
      i = i + 1;
    }
  }
  return matched;
}

static uint8_t process_file(
  const char* path,
  char** keywords,
  uint32_t keyword_count,
  uint8_t case_insensitive,
  uint8_t match_mode,
  uint8_t line_mode,
  uint8_t list_mode,
  uint8_t negate,
  pthread_mutex_t* output_mutex)
{
  FILE* file;
  char* line;
  size_t line_capacity;
  char* match_line;
  size_t match_capacity;
  ssize_t line_length_signed;
  size_t line_length;
  uint8_t matched_raw;
  uint8_t matched;
  uint8_t* keyword_seen;
  uint32_t keyword_seen_count;
  uint32_t i;
  file = fopen(path, "rb");
  if (!file) return 0;
  line = 0;
  line_capacity = 0;
  match_line = 0;
  match_capacity = 0;
  matched_raw = 0;
  keyword_seen = 0;
  keyword_seen_count = 0;
  if (line_mode == line_mode_file && match_mode == match_mode_all) {
    keyword_seen = calloc(keyword_count, sizeof(uint8_t));
    if (!keyword_seen) {
      fclose(file);
      return 0;
    }
  }
  for (;;) {
    line_length_signed = getline(&line, &line_capacity, file);
    if (line_length_signed < 0) break;
    line_length = (size_t)line_length_signed;
    if (case_insensitive) {
      if (match_capacity < line_length + 1) {
        char* new_match_line;
        size_t new_capacity;
        new_capacity = line_length + 1;
        new_match_line = realloc(match_line, new_capacity);
        if (!new_match_line) {
          free(line);
          free(match_line);
          free(keyword_seen);
          fclose(file);
          return 0;
        }
        match_line = new_match_line;
        match_capacity = new_capacity;
      }
      memcpy(match_line, line, line_length);
      match_line[line_length] = 0;
      lowercase_in_place(match_line);
    } else {
      match_line = line;
    }
    if (line_mode == line_mode_line) {
      uint8_t line_matched;
      line_matched = match_buffer(
        match_line, line_length, keywords, keyword_count, match_mode);
      if (line_matched) {
        matched_raw = 1;
        if (list_mode == list_mode_paths_and_lines && !negate) {
          pthread_mutex_lock(output_mutex);
          write(1, path, strlen(path));
          write(1, ": ", 2);
          write(1, line, line_length);
          pthread_mutex_unlock(output_mutex);
        }
        if (list_mode == list_mode_paths_only) break;
      }
    } else {
      if (match_mode == match_mode_all) {
        i = 0;
        while (i < keyword_count) {
          if (!keyword_seen[i]) {
            if (strstr(match_line, keywords[i]) != 0) {
              keyword_seen[i] = 1;
              keyword_seen_count = keyword_seen_count + 1;
            }
          }
          i = i + 1;
        }
        if (keyword_seen_count == keyword_count) matched_raw = 1;
      } else {
        if (!matched_raw) {
          i = 0;
          while (i < keyword_count) {
            if (strstr(match_line, keywords[i]) != 0) {
              matched_raw = 1;
              break;
            }
            i = i + 1;
          }
        }
      }
      if (list_mode == list_mode_paths_and_lines && !negate) {
        uint8_t line_has_keyword;
        line_has_keyword = 0;
        i = 0;
        while (i < keyword_count) {
          if (strstr(match_line, keywords[i]) != 0) {
            line_has_keyword = 1;
            break;
          }
          i = i + 1;
        }
        if (line_has_keyword) {
          pthread_mutex_lock(output_mutex);
          write(1, path, strlen(path));
          write(1, ": ", 2);
          write(1, line, line_length);
          pthread_mutex_unlock(output_mutex);
        }
      }
      if (matched_raw && list_mode == list_mode_paths_only) {
        if (match_mode == match_mode_some) break;
        if (match_mode == match_mode_all && keyword_seen_count == keyword_count)
          break;
      }
    }
  }
  free(line);
  if (case_insensitive) free(match_line);
  free(keyword_seen);
  fclose(file);
  matched = matched_raw;
  if (negate) matched = matched ? 0 : 1;
  if (matched) {
    if (list_mode == list_mode_paths_only || negate) {
      size_t path_length;
      path_length = strlen(path);
      if (path_length > 0) {
        pthread_mutex_lock(output_mutex);
        write(1, path, path_length);
        write(1, "\n", 1);
        pthread_mutex_unlock(output_mutex);
      }
    }
  }
  return matched;
}

static void* worker_thread(void* argument)
{
  worker_context_t* context;
  size_t index;
  struct stat stat_buffer;
  context = (worker_context_t*)argument;
  for (;;) {
    pthread_mutex_lock(&context->index_mutex);
    index = context->next_index;
    if (index >= context->paths_used) {
      pthread_mutex_unlock(&context->index_mutex);
      break;
    }
    context->next_index = index + 1;
    pthread_mutex_unlock(&context->index_mutex);
    if (lstat(context->paths[index], &stat_buffer) != 0) continue;
    if (!S_ISREG(stat_buffer.st_mode)) continue;
    process_file(
      context->paths[index],
      context->keywords,
      context->keyword_count,
      context->case_insensitive,
      context->match_mode,
      context->line_mode,
      context->list_mode,
      context->negate,
      &context->output_mutex);
  }
  return 0;
}

int main(int argc, char** argv)
{
  simple_option_parser_declare(options);
  uint32_t rest_index;
  uint32_t argc_u32;
  char** argv_shifted;
  uint32_t keyword_count;
  char** keywords;
  uint8_t case_insensitive;
  uint8_t match_mode;
  uint8_t line_mode;
  uint8_t list_mode;
  uint8_t negate;
  size_t i;
  char** paths;
  size_t paths_used;
  char* paths_data;
  worker_context_t context;
  long cpu_count;
  uint32_t thread_count;
  pthread_t threads[256];

  simple_option_parser_set_option(options, 'a', 0);
  simple_option_parser_set_option(options, 'o', 0);
  simple_option_parser_set_option(options, 'n', 0);
  simple_option_parser_set_option(options, 'm', 0);
  simple_option_parser_set_option(options, 'l', 0);

  if (argc <= 1) return 0;

  argc_u32 = (uint32_t)(argc - 1);
  argv_shifted = argv + 1;
  if (!simple_option_parser(argc_u32, argv_shifted, options, &rest_index))
    return 1;
  if (rest_index >= argc_u32) return 0;

  keywords = argv_shifted + rest_index;
  keyword_count = argc_u32 - rest_index;

  case_insensitive = keywords_case_insensitive(keywords, keyword_count);
  if (case_insensitive) {
    uint32_t index = 0;
    while (index < keyword_count) {
      lowercase_in_place(keywords[index]);
      index = index + 1;
    }
  }

  match_mode = match_mode_all;
  if (options[(uint8_t)'o'].is_present) match_mode = match_mode_some;

  line_mode = line_mode_line;
  if (options[(uint8_t)'m'].is_present) line_mode = line_mode_file;

  list_mode = list_mode_paths_only;
  if (options[(uint8_t)'l'].is_present) list_mode = list_mode_paths_and_lines;

  negate = options[(uint8_t)'n'].is_present ? 1 : 0;

  paths = 0;
  paths_used = 0;
  paths_data = read_paths('\n', &paths, &paths_used);
  if (!paths_data) return 0;

  context.paths = paths;
  context.paths_used = paths_used;
  context.keywords = keywords;
  context.keyword_count = keyword_count;
  context.case_insensitive = case_insensitive;
  context.match_mode = match_mode;
  context.line_mode = line_mode;
  context.list_mode = list_mode;
  context.negate = negate;
  context.next_index = 0;

  pthread_mutex_init(&context.index_mutex, 0);
  pthread_mutex_init(&context.output_mutex, 0);

  cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  if (cpu_count <= 0) thread_count = 1;
  else thread_count = (uint32_t)cpu_count;

  if (thread_count > 256) thread_count = 256;
  if ((size_t)thread_count > paths_used) thread_count = (uint32_t)paths_used;
  if (thread_count == 0) {
    pthread_mutex_destroy(&context.index_mutex);
    pthread_mutex_destroy(&context.output_mutex);
    free(paths);
    free(paths_data);
    return 0;
  }

  for (i = 0; i < thread_count; i += 1) {
    pthread_create(&threads[i], 0, worker_thread, &context);
  }
  for (i = 0; i < thread_count; i += 1) {
    pthread_join(threads[i], 0);
  }

  pthread_mutex_destroy(&context.index_mutex);
  pthread_mutex_destroy(&context.output_mutex);

  free(paths);
  free(paths_data);
  return 0;
}
