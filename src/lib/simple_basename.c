uint8_t* simple_basename(uint8_t* path) {
  uint8_t* slash_pointer = strrchr(path, '/');
  return((slash_pointer ? (1 + slash_pointer) : path));
}
