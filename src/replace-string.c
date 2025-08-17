#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <dirent.h>

#define maximum_path_length 4096
#define binary_probe 1024

static uint8_t* read_entire_file(const char* path, size_t* out_len) {
  FILE* f = fopen(path, "rb");
  if (!f) return 0;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
  long size = ftell(f);
  if (size < 0) { fclose(f); return 0; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
  uint8_t* data = malloc((size_t)size + 1);
  if (!data) { fclose(f); return 0; }
  size_t readn = fread(data, 1, (size_t)size, f);
  fclose(f);
  if (readn != (size_t)size) { free(data); return 0; }
  data[readn] = 0;
  *out_len = readn;
  return data;
}

static int is_text_file(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return 0;
  uint8_t probe[binary_probe];
  size_t n = fread(probe, 1, binary_probe, f);
  fclose(f);
  for (size_t i = 0; i < n; i++) if (probe[i] == 0) return 0;
  return 1;
}

static uint8_t* replace_all(const uint8_t* src, size_t src_len, const uint8_t* pat, size_t pat_len, const uint8_t* rep, size_t rep_len) {
  if (pat_len == 0) return 0;
  size_t count = 0;
  const uint8_t* p = src;
  const uint8_t* hit;
  while ((hit = (const uint8_t*)strstr((const char*)p, (const char*)pat))) { count++; p = hit + pat_len; }
  if (count == 0) return 0;
  size_t out_len = src_len + count * (rep_len - pat_len);
  uint8_t* out = malloc(out_len + 1);
  if (!out) return 0;
  uint8_t* d = out;
  p = src;
  while ((hit = (const uint8_t*)strstr((const char*)p, (const char*)pat))) {
    size_t seg = (size_t)(hit - p);
    memcpy(d, p, seg); d += seg;
    memcpy(d, rep, rep_len); d += rep_len;
    p = hit + pat_len;
  }
  size_t tail = (size_t)((src + src_len) - p);
  memcpy(d, p, tail); d += tail;
  *d = 0;
  return out;
}

static int printed_any_block = 0;

static void print_block_line(const char* s) {
  if (printed_any_block) fwrite("\n", 1, 1, stdout);
  fputs(s, stdout);
  printed_any_block = 1;
}

static void preview_line_diffs(const char* path, const uint8_t* src, const uint8_t* pat, size_t pat_len, const uint8_t* rep, size_t rep_len) {
  const uint8_t* line_start = src;
  while (*line_start) {
    const uint8_t* line_end = (const uint8_t*)strchr((const char*)line_start, '\n');
    size_t line_len = line_end ? (size_t)(line_end - line_start + 1) : strlen((const char*)line_start);
    uint8_t* line_buf = malloc(line_len + 1);
    if (!line_buf) return;
    memcpy(line_buf, line_start, line_len);
    line_buf[line_len] = 0;
    uint8_t* changed = replace_all(line_buf, line_len, pat, pat_len, rep, rep_len);
    if (changed) {
      if (line_len && line_buf[line_len - 1] == '\n') line_buf[line_len - 1] = 0;
      print_block_line(path);
      print_block_line((const char*)line_buf);
      print_block_line((const char*)changed);
      free(changed);
    }
    free(line_buf);
    if (!line_end) break;
    line_start = line_end + 1;
  }
}

static void process_file(const char* path, int write_changes, const uint8_t* pat, size_t pat_len, const uint8_t* rep, size_t rep_len) {
  if (!is_text_file(path)) return;
  size_t src_len = 0;
  uint8_t* src = read_entire_file(path, &src_len);
  if (!src) return;
  uint8_t* replaced = replace_all(src, src_len, pat, pat_len, rep, rep_len);
  if (!replaced) { free(src); return; }
  if (write_changes) {
    FILE* wf = fopen(path, "wb");
    if (wf) { fwrite(replaced, 1, strlen((char*)replaced), wf); fclose(wf); }
  } else {
    preview_line_diffs(path, src, pat, pat_len, rep, rep_len);
  }
  free(src);
  free(replaced);
}

static void join_paths(char* out, size_t out_size, const char* base, const char* name) {
  size_t base_len = strlen(base);
  if (base_len && base[base_len - 1] == '/')
    snprintf(out, out_size, "%s%s", base, name);
  else
    snprintf(out, out_size, "%s/%s", base, name);
}

static void traverse(const char* path, int write_changes, const uint8_t* pat, size_t pat_len, const uint8_t* rep, size_t rep_len) {
  struct stat st;
  if (lstat(path, &st) == -1) return;
  if (S_ISDIR(st.st_mode)) {
    DIR* dir = opendir(path);
    if (!dir) return;
    struct dirent* ent;
    while ((ent = readdir(dir))) {
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
      char subpath[maximum_path_length];
      join_paths(subpath, sizeof(subpath), path, ent->d_name);
      traverse(subpath, write_changes, pat, pat_len, rep, rep_len);
    }
    closedir(dir);
  } else if (S_ISREG(st.st_mode)) {
    process_file(path, write_changes, pat, pat_len, rep, rep_len);
  }
}

static void print_usage(void) {
  fputs("usage: replacer [-w] <pattern> <replacement> [paths...]\n", stdout);
  fputs("-w: write changes to files (instead of preview)\n", stdout);
}

int main(int argc, char** argv) {
  if (argc == 1) { print_usage(); return 1; }
  int write_changes = 0;
  int opt;
  while ((opt = getopt(argc, argv, "w")) != -1) {
    if (opt == 'w') write_changes = 1;
    else { print_usage(); return 1; }
  }
  if (argc - optind < 2) { print_usage(); return 1; }
  uint8_t* pat = (uint8_t*)argv[optind++];
  uint8_t* rep = (uint8_t*)argv[optind++];
  size_t pat_len = strlen((char*)pat);
  size_t rep_len = strlen((char*)rep);
  if (pat_len == 0) return 1;
  if (optind == argc)
    traverse(".", write_changes, pat, pat_len, rep, rep_len);
  else
    for (int i = optind; i < argc; i++) traverse(argv[i], write_changes, pat, pat_len, rep, rep_len);
  return 0;
}