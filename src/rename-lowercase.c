#include <stdio.h>  // rename
#include <string.h>  // strlen
#include <unistd.h>  // write
#include <inttypes.h>
#include <errno.h>
#include "lib/print_errno.c"

#define max_path_len 4096

const uint8_t case_table[] = {
  ['A'] = 'a', ['B'] = 'b', ['C'] = 'c', ['D'] = 'd', ['E'] = 'e', ['F'] = 'f',
  ['G'] = 'g', ['H'] = 'h', ['I'] = 'i', ['J'] = 'j', ['K'] = 'k', ['L'] = 'l',
  ['M'] = 'm', ['N'] = 'n', ['O'] = 'o', ['P'] = 'p', ['Q'] = 'q', ['R'] = 'r',
  ['S'] = 's', ['T'] = 't', ['U'] = 'u', ['V'] = 'v', ['W'] = 'w', ['X'] = 'x',
  ['Y'] = 'y', ['Z'] = 'z'
};

uint8_t help_text[] = "arguments: path ...\n";
uint8_t long_path_text[] = "path too long\n";
uint8_t exists_text[] = "target already exists\n";

int main(int argc, char** argv) {
  if (argc < 2) {
    write(1, help_text, sizeof(help_text));
    return 0;
  }
  size_t len;
  size_t j;
  uint8_t new[max_path_len + 1];
  uint8_t modified;
  for (int i = 1; i < argc; i += 1) {
    // check if path too short or too long
    const uint8_t* old = argv[i];
    len = strlen(old);
    if (!len) continue;
    if (max_path_len < len) {
      write(2, long_path_text, sizeof(long_path_text));
      return 1;
    }
    modified = 0;
    new[len] = 0;
    // copy basename and change casing
    j = len;
    while (j && old[j - 1] != '/') {
      j -= 1;
      if ('A' <= old[j] && old[j] <= 'Z') {
        modified = 1;
        new[j] = case_table[old[j]];
      }
      else new[j] = old[j];
    }
    if (!modified) continue;
    // copy dirname
    while (j) {
      j -= 1;
      new[j] = old[j];
    }
    // skip if target exists
    if (!access(new, F_OK)) {
      write(2, exists_text, sizeof(exists_text));
      continue;
    }
    // attempt rename
    write(1, old, len);
    write(1, " -> ", 4);
    write(1, new, len);
    write(1, "\n", 1);
    if (rename(old, new)) {
      print_errno();
      return 1;
    }
  }
  return 0;
}
