#define ensure_directory_structure_max_depth 255

int ensure_directory_structure(uint8_t* path) {
  /* modifies path temporarily. path may stay modified on error.
    test cases: //aa, //, /, aa, aa/bb/cc, aa/bb/, aa/bb//.
    algorithm:
      phase 1: count slashes backwards, set each to the string-ending null byte
        and memorize the position until the shortest existing path has been found.
      phase 2: extend the path from parent to child by re-adding the slashes */
  uint16_t slashes[ensure_directory_structure_max_depth];
  uint16_t slashes_len = 0;
  uint16_t path_len, i;
  if (!access(path, X_OK)) return 0;
  path_len = strlen(path);
  // phase 1: find first existing parent directory
  i = path_len - 1;
  // do not check 'i == 0' because a beginning single character segment is either root or relative
  while (i > 1) {
    i -= 1;
    // skip repeated slashes
    if ('/' != path[i] || '/' == path[i + 1]) continue;
    // on error, the path may be in a shortened state. we could rollback modifications here
    if (slashes_len == ensure_directory_structure_max_depth) return -1;
    slashes[slashes_len] = i;
    slashes_len += 1;
    path[i] = 0;
    if (!access(path, X_OK)) break;
  }
  // mode 0777 to respect users umask
  if (!slashes_len) return mkdir(path, 0777);
  // phase 2: create child directories
  while (slashes_len) {
    slashes_len -= 1;
    path[slashes[slashes_len]] = '/';
    if (mkdir(path, 0777)) return -1;
  }
  return 0;
}
