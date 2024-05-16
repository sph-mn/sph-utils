#include <unistd.h>  // write()
#define maxlen (unsigned short int)-1

int main(int argc, char** argv) {
  // handles:
  // * null strings
  // * strings without extensions
  // * strings that end with a dot
  // * strings that start with a dot (first dot does not count as extension separator)
  if (2 == argc) {
    unsigned short int len = 0;
    unsigned short int i = 0;
    do {
      if (!argv[1][i]) {
        if (!len) len = i;
        break;
      }
      else if ('.' == argv[1][i]) len = i;
      i += 1;
    } while(i < maxlen);
    // may replace the final null byte
    argv[1][len] = '\n';
    write(1, argv[1], len + 1);
  }
  else {
    unsigned char* help_text = "arguments: filename\n";
    write(1, help_text, 20);
    return 2;
  }
}
