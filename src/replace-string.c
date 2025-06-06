#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <dirent.h>

#define maximum_path_length 4096
#define binary_probe 1024

static uint8_t* read_file(const char* path, size_t* len) {
  FILE* fp = fopen(path, "rb");
  if (!fp) return 0;
  fseek(fp, 0, SEEK_END);
  *len = ftell(fp);
  rewind(fp);
  uint8_t* data = malloc(*len + 1);
  if (!data) { fclose(fp); return 0; }
  if (fread(data, 1, *len, fp) != *len) { fclose(fp); free(data); return 0; }
  data[*len] = 0;
  fclose(fp);
  return data;
}

static uint8_t* replace_all(const uint8_t* src, const uint8_t* pat, const uint8_t* rep) {
  size_t src_len = strlen((const char*)src);
  size_t pat_len = strlen((const char*)pat);
  if (pat_len == 0) return 0;
  size_t rep_len = strlen((const char*)rep);
  size_t count = 0;
  const uint8_t* p = src;
  while ((p = (const uint8_t*)strstr((const char*)p, (const char*)pat))) { count += 1; p += pat_len; }
  if (count == 0) return 0;
  size_t dst_len = src_len + count * (rep_len - pat_len);
  uint8_t* dst = malloc(dst_len + 1);
  if (!dst) return 0;
  uint8_t* d = dst;
  p = src;
  const uint8_t* q;
  while ((q = (const uint8_t*)strstr((const char*)p, (const char*)pat))) {
    size_t seg = q - p;
    memcpy(d, p, seg); d += seg;
    memcpy(d, rep, rep_len); d += rep_len;
    p = q + pat_len;
  }
  strcpy((char*)d, (const char*)p);
  return dst;
}

static void preview_line_changes(const char* path, const uint8_t* src, const uint8_t* pat, const uint8_t* rep) {
  const uint8_t* s = src;
  while (*s) {
    const uint8_t* line_end = (const uint8_t*)strchr((const char*)s, '\n');
    size_t len = line_end ? (size_t)(line_end - s + 1) : strlen((const char*)s);
    uint8_t* line = malloc(len + 1);
    memcpy(line, s, len); line[len] = 0;
    uint8_t* changed = replace_all(line, pat, rep);
    if (changed) {
      if (line[len - 1] == '\n') line[len - 1] = 0;
      printf("%s\n%s\n%s\n", path, line, changed);
      free(changed);
    }
    free(line);
    if (!line_end) break;
    s = line_end + 1;
  }
}

static void process_file(const char* path, uint8_t inplace, const uint8_t* pat, const uint8_t* rep) {
  FILE* fp = fopen(path, "rb");
  if (!fp) return;
  uint8_t probe[binary_probe];
  size_t probe_len = fread(probe, 1, binary_probe, fp);
  for (size_t i = 0; i < probe_len; i += 1) { if (probe[i] == 0) { fclose(fp); return; } }
  fclose(fp);
  size_t src_len = 0;
  uint8_t* src = read_file(path, &src_len);
  if (!src) return;
  uint8_t* dst = replace_all(src, pat, rep);
  if (!dst) { free(src); return; }
  if (inplace) {
    FILE* wf = fopen(path, "wb");
    if (wf) { fwrite(dst, 1, strlen((char*)dst), wf); fclose(wf); }
  } else {
    preview_line_changes(path, src, pat, rep);
  }
  free(src); free(dst);
}

static void join_path(char* out, size_t sz, const char* base, const char* name) {
  size_t len = strlen(base);
  if (len && base[len - 1] == '/')
    snprintf(out, sz, "%s%s", base, name);
  else
    snprintf(out, sz, "%s/%s", base, name);
}

static void traverse(const char* path, uint8_t inplace, const uint8_t* pat, const uint8_t* rep) {
  struct stat st;
  if (lstat(path, &st) == -1) return;
  if (S_ISDIR(st.st_mode)) {
    DIR* dir = opendir(path);
    if (!dir) return;
    struct dirent* ent;
    while ((ent = readdir(dir))) {
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
      char sub[maximum_path_length];
      join_path(sub, sizeof(sub), path, ent->d_name);
      traverse(sub, inplace, pat, rep);
    }
    closedir(dir);
  } else if (S_ISREG(st.st_mode)) {
    process_file(path, inplace, pat, rep);
  }
}

int main(int argc, char** argv) {
  uint8_t inplace = 0;
  int opt;
  while ((opt = getopt(argc, argv, "g")) != -1) {
    if (opt == 'g') inplace = 1;
    else return 1;
  }
  if (argc - optind < 2) return 1;
  uint8_t* pat = (uint8_t*)argv[optind++];
  uint8_t* rep = (uint8_t*)argv[optind++];
  if (strlen((char*)pat) == 0) return 1;

  if (optind == argc) {
    traverse(".", inplace, pat, rep);
  } else {
    for (int i = optind; i < argc; i += 1) traverse(argv[i], inplace, pat, rep);
  }
  return 0;
}
