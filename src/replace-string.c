#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <dirent.h>

#define replace_string_maximum_path_length 4096
#define replace_string_binary_probe 1024

struct replace_string_ancestor {
  dev_t device;
  ino_t inode;
  struct replace_string_ancestor* parent;
};

uint8_t* replace_string_read_entire_file(char* path, size_t* out_len) {
  FILE* file;
  long size;
  uint8_t* data;
  size_t read_count;

  file = fopen(path, "rb");
  if (!file) {
    return 0;
  }

  if (fseek(file, 0, SEEK_END)) {
    fclose(file);
    return 0;
  }

  size = ftell(file);
  if (size < 0) {
    fclose(file);
    return 0;
  }

  if (fseek(file, 0, SEEK_SET)) {
    fclose(file);
    return 0;
  }

  data = malloc(size + 1);
  if (!data) {
    fclose(file);
    return 0;
  }

  read_count = fread(data, 1, size, file);
  fclose(file);

  if (read_count != size) {
    free(data);
    return 0;
  }

  data[read_count] = 0;
  *out_len = read_count;
  return data;
}

int replace_string_is_text_file(char* path) {
  FILE* file;
  uint8_t probe[replace_string_binary_probe];
  size_t count;
  size_t index;

  file = fopen(path, "rb");
  if (!file) {
    return 0;
  }

  count = fread(probe, 1, sizeof(probe), file);
  fclose(file);

  for (index = 0; index < count; index += 1) {
    if (!probe[index]) {
      return 0;
    }
  }

  return 1;
}

uint8_t* replace_string_replace_all(
  uint8_t* source, size_t source_length,
  uint8_t* pattern, size_t pattern_length,
  uint8_t* replacement, size_t replacement_length
) {
  size_t count;
  size_t output_length;
  size_t segment_length;
  size_t tail_length;
  uint8_t* position;
  uint8_t* hit;
  uint8_t* output;
  uint8_t* destination;

  count = 0;
  position = source;

  while ((hit = (uint8_t*)strstr((char*)position, (char*)pattern))) {
    count += 1;
    position = hit + pattern_length;
  }

  if (!count) {
    return 0;
  }

  output_length = source_length + count * (replacement_length - pattern_length);
  output = malloc(output_length + 1);
  if (!output) {
    return 0;
  }

  destination = output;
  position = source;

  while ((hit = (uint8_t*)strstr((char*)position, (char*)pattern))) {
    segment_length = hit - position;
    memcpy(destination, position, segment_length);
    destination += segment_length;

    memcpy(destination, replacement, replacement_length);
    destination += replacement_length;

    position = hit + pattern_length;
  }

  tail_length = source + source_length - position;
  memcpy(destination, position, tail_length);
  destination += tail_length;
  *destination = 0;

  return output;
}

int replace_string_printed_any_block = 0;

void replace_string_print_block_line(char* text) {
  if (replace_string_printed_any_block) {
    fwrite("\n", 1, 1, stdout);
  }

  fputs(text, stdout);
  replace_string_printed_any_block = 1;
}

void replace_string_preview_line_diffs(
  char* path,
  uint8_t* source,
  uint8_t* pattern, size_t pattern_length,
  uint8_t* replacement, size_t replacement_length
) {
  uint8_t* line_start;
  uint8_t* line_end;
  uint8_t* line_buffer;
  uint8_t* changed;
  size_t line_length;

  line_start = source;

  while (*line_start) {
    line_end = (uint8_t*)strchr((char*)line_start, '\n');
    line_length = line_end ? line_end - line_start + 1 : strlen((char*)line_start);

    line_buffer = malloc(line_length + 1);
    if (!line_buffer) {
      return;
    }

    memcpy(line_buffer, line_start, line_length);
    line_buffer[line_length] = 0;

    changed = replace_string_replace_all(
      line_buffer, line_length,
      pattern, pattern_length,
      replacement, replacement_length
    );

    if (changed) {
      if (line_length && line_buffer[line_length - 1] == '\n') {
        line_buffer[line_length - 1] = 0;
      }

      replace_string_print_block_line(path);
      replace_string_print_block_line((char*)line_buffer);
      replace_string_print_block_line((char*)changed);
      free(changed);
    }

    free(line_buffer);

    if (line_end) {
      line_start = line_end + 1;
    } else {
      break;
    }
  }
}

void replace_string_process_file(
  char* path, int write_changes,
  uint8_t* pattern, size_t pattern_length,
  uint8_t* replacement, size_t replacement_length
) {
  size_t source_length;
  uint8_t* source;
  uint8_t* replaced;
  FILE* file;

  if (!replace_string_is_text_file(path)) {
    return;
  }

  source = replace_string_read_entire_file(path, &source_length);
  if (!source) {
    return;
  }

  replaced = replace_string_replace_all(
    source, source_length,
    pattern, pattern_length,
    replacement, replacement_length
  );

  if (!replaced) {
    free(source);
    return;
  }

  if (write_changes) {
    file = fopen(path, "wb");
    if (file) {
      fwrite(replaced, 1, strlen((char*)replaced), file);
      fclose(file);
    }
  } else {
    replace_string_preview_line_diffs(
      path, source,
      pattern, pattern_length,
      replacement, replacement_length
    );
  }

  free(source);
  free(replaced);
}

void replace_string_join_paths(
  char* output, size_t output_size,
  char* base, char* name
) {
  size_t base_length;

  base_length = strlen(base);

  if (base_length && base[base_length - 1] == '/') {
    snprintf(output, output_size, "%s%s", base, name);
  } else {
    snprintf(output, output_size, "%s/%s", base, name);
  }
}

void replace_string_traverse(
  char* path, int write_changes,
  uint8_t* pattern, size_t pattern_length,
  uint8_t* replacement, size_t replacement_length,
  struct replace_string_ancestor* ancestors
) {
  struct stat status;
  struct replace_string_ancestor current;
  struct replace_string_ancestor* ancestor;
  struct dirent* entry;
  DIR* directory;
  char subpath[replace_string_maximum_path_length];

  if (stat(path, &status) == -1) {
    return;
  }

  if (S_ISDIR(status.st_mode)) {
    for (ancestor = ancestors; ancestor; ancestor = ancestor->parent) {
      if (ancestor->device == status.st_dev && ancestor->inode == status.st_ino) {
        return;
      }
    }

    directory = opendir(path);
    if (!directory) {
      return;
    }

    current.device = status.st_dev;
    current.inode = status.st_ino;
    current.parent = ancestors;

    while ((entry = readdir(directory))) {
      if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
        replace_string_join_paths(
          subpath, sizeof(subpath), path, entry->d_name
        );

        replace_string_traverse(
          subpath, write_changes,
          pattern, pattern_length,
          replacement, replacement_length,
          &current
        );
      }
    }

    closedir(directory);
  } else if (S_ISREG(status.st_mode)) {
    replace_string_process_file(
      path, write_changes,
      pattern, pattern_length,
      replacement, replacement_length
    );
  }
}

void replace_string_print_usage(void) {
  fputs("arguments: [-w] <pattern> <replacement> [paths...]\n", stdout);
  fputs("-w: write changes to files (instead of preview)\n", stdout);
}

int main(int argc, char** argv) {
  int write_changes;
  int option;
  int index;
  uint8_t* pattern;
  uint8_t* replacement;
  size_t pattern_length;
  size_t replacement_length;

  if (argc == 1) {
    replace_string_print_usage();
    return 1;
  }

  write_changes = 0;

  while ((option = getopt(argc, argv, "w")) != -1) {
    if (option == 'w') {
      write_changes = 1;
    } else {
      replace_string_print_usage();
      return 1;
    }
  }

  if (argc - optind < 2) {
    replace_string_print_usage();
    return 1;
  }

  pattern = (uint8_t*)argv[optind];
  replacement = (uint8_t*)argv[optind + 1];
  pattern_length = strlen((char*)pattern);
  replacement_length = strlen((char*)replacement);

  if (!pattern_length) {
    return 1;
  }

  optind += 2;

  if (optind == argc) {
    replace_string_traverse(
      ".", write_changes,
      pattern, pattern_length,
      replacement, replacement_length,
      0
    );
  } else {
    for (index = optind; index < argc; index += 1) {
      replace_string_traverse(
        argv[index], write_changes,
        pattern, pattern_length,
        replacement, replacement_length,
        0
      );
    }
  }

  return 0;
}