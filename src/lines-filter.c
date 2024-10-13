#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define mode_and 0
#define mode_or 1

int parse_options(int argc, char** argv, int* mode, int* negate, char*** patterns, int* num_patterns) {
  int opt;
  while ((opt = getopt(argc, argv, "aon")) != -1) {
    switch (opt) {
    case 'a':
      *mode = mode_and;
      break;
    case 'o':
      *mode = mode_or;
      break;
    case 'n':
      *negate = 1;
      break;
    default:
      return -1;
    }
  }

  *num_patterns = argc - optind;
  if (*num_patterns > 0) {
    *patterns = argv + optind;
  } else {
    *patterns = 0;
  }
  return 0;
}

int match_line(const char* line, char** patterns, int num_patterns, int mode) {
  int i;
  if (mode == mode_and) {
    for (i = 0; i < num_patterns; i++) {
      if (strstr(line, patterns[i]) == 0)
        return 0;
    }
    return 1;
  } else if (mode == mode_or) {
    for (i = 0; i < num_patterns; i++) {
      if (strstr(line, patterns[i]) != 0)
        return 1;
    }
    return 0;
  } else {
    return 0;
  }
}

void usage(const char* progname) {
  fprintf(stderr, "usage: %s [options] arguments ...\n", progname);
  fprintf(stderr, "  -a  matching lines must contain all strings. the default\n");
  fprintf(stderr, "  -n  negate\n");
  fprintf(stderr, "  -o  matching lines must contain at least one of the strings\n");
}

int main(int argc, char** argv) {
  int mode = mode_and;
  int negate = 0;
  char** patterns = 0;
  int num_patterns = 0;
  char line[8192];
  int matched;

  if (parse_options(argc, argv, &mode, &negate, &patterns, &num_patterns) != 0) {
    usage(argv[0]);
    return 1;
  }

  if (num_patterns == 0) {
    fprintf(stderr, "no search patterns specified.\n");
    usage(argv[0]);
    return 1;
  }

  while (fgets(line, sizeof(line), stdin) != 0) {
    matched = match_line(line, patterns, num_patterns, mode);
    if (negate)
      matched = !matched;
    if (matched)
      fputs(line, stdout);
  }

  // no need to free patterns, as they point into argv
  return 0;
}
