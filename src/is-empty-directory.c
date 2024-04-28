#include <dirent.h>
#include <errno.h>

int main(int argc, char** argv) {
  DIR* dir;
  for (unsigned int i = 1; i < argc; i += 1) {
    dir = opendir(argv[i]);  // closed on process termination
    if ((readdir(dir) && readdir(dir) && readdir(dir)) || errno) return(1);
  }
  return 0;
}
