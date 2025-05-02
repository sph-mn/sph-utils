// experimental thumbnailer implementation for file-managers like thunar.
// existing thumbnailers start creating thumbnails only when a file icon is displayed.
// that is too late - the user waits too long time till the thumbnails are loaded.
// this thumbnailer would supposed to be usable for more aggressive thumbnail generation.

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include "md5.c"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + PATH_MAX))
struct thumbsize { char *name; int size; };
struct thumbsize thumb_sizes[] = { { "normal", 256 }, { "large", 512 }, { "x-large", 768 }, { "xx-large", 1024 } };
int num_sizes = sizeof(thumb_sizes) / sizeof(thumb_sizes[0]);

struct watch { int wd; char path[PATH_MAX]; };
#define MAX_WATCHES 1024
struct watch watches[MAX_WATCHES];
int num_watches = 0;

int is_image(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext)return 0;
  ext++;
  char lower[16];
  int i = 0;
  while(ext[i] && i < 15){ lower[i] = tolower((unsigned char)ext[i]); i++; }
  lower[i] = '\0';
  return (!strcmp(lower, "jpg") || !strcmp(lower, "jpeg") || !strcmp(lower, "png") || !strcmp(lower, "gif") || !strcmp(lower, "webp"));
}

void make_thumb(const char *path) {
  char hash[33];
  md5_string(path, hash);
  char *home_dir = getenv("HOME");
  if (!home_dir)return;
  for (int i = 0; i < num_sizes; i++) {
    char thumb[PATH_MAX];
    snprintf(thumb, sizeof(thumb), "%s/.cache/thumbnails/%s/%s.png", home_dir, thumb_sizes[i].name, hash);
    if (access(thumb, F_OK) != 0) {
      char cmd[PATH_MAX * 2];
      snprintf(cmd, sizeof(cmd), "ffmpegthumbnailer -i \"%s\" -o \"%s\" -s %d", path, thumb, thumb_sizes[i].size);
      system(cmd);
    }
  }
}

void initial_pass(const char *dir_path) {
  DIR *dp = opendir(dir_path);
  if (!dp)return;
  struct dirent *entry;
  while ((entry = readdir(dp))) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    char fullpath[PATH_MAX];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, entry->d_name);
    struct stat st;
    if (stat(fullpath, &st) < 0) continue;
    if (S_ISDIR(st.st_mode))
      initial_pass(fullpath);
    else if (S_ISREG(st.st_mode) && is_image(fullpath))
      make_thumb(fullpath);
  }
  closedir(dp);
}

void add_watches(int fd, const char *dir_path) {
  int wd = inotify_add_watch(fd, dir_path, IN_CREATE | IN_OPEN);
  if (wd < 0)return;
  if (num_watches < MAX_WATCHES) {
    watches[num_watches].wd = wd;
    strncpy(watches[num_watches].path, dir_path, PATH_MAX);
    num_watches++;
  }
  DIR *dp = opendir(dir_path);
  if (!dp)return;
  struct dirent *entry;
  while ((entry = readdir(dp))) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    char fullpath[PATH_MAX];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, entry->d_name);
    struct stat st;
    if (stat(fullpath, &st) < 0) continue;
    if (S_ISDIR(st.st_mode))
      add_watches(fd, fullpath);
  }
  closedir(dp);
}

int main(int argc, char **argv) {
  if (argc < 2)return 1;
  initial_pass(argv[1]);
  int fd = inotify_init1(0);
  if (fd < 0)return 1;
  add_watches(fd, argv[1]);
  char buffer[EVENT_BUF_LEN];
  while (1) {
    int length = read(fd, buffer, EVENT_BUF_LEN);
    if (length < 0) continue;
    int i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      char base_path[PATH_MAX] = "";
      for (int j = 0; j < num_watches; j++) {
        if (watches[j].wd == event->wd) {
          strncpy(base_path, watches[j].path, PATH_MAX);
          break;
        }
      }
      if (base_path[0] == '\0')
        strncpy(base_path, argv[1], PATH_MAX);
      char filepath[PATH_MAX];
      if (event->len)
        snprintf(filepath, sizeof(filepath), "%s/%s", base_path, event->name);
      else
        strncpy(filepath, base_path, PATH_MAX);
      struct stat st;
      if (stat(filepath, &st) < 0) { i += sizeof(struct inotify_event) + event->len; continue; }
      if (event->mask & IN_ISDIR) {
        if (event->mask & IN_CREATE)
          add_watches(fd, filepath);
      } else {
        if (is_image(filepath))
          make_thumb(filepath);
      }
      i += sizeof(struct inotify_event) + event->len;
    }
  }
  close(fd);
  return 0;
}
