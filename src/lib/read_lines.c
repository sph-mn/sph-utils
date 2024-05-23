#ifndef read_lines_config
#define read_lines_expected_line_length 400
#define read_lines_initial_line_count 10000
#define read_lines_chunk_size read_lines_initial_line_count * read_lines_expected_line_length
#define read_lines_growth_factor 3
#define read_lines_config
#define read_lines_data_size_t uint32_t
#define read_lines_overlap_size_t uint16_t
#endif

typedef struct {
  uint8_t delimiter;
  uint8_t* data;
  uint32_t data_size;
  uint32_t data_used;
  uint16_t* lines;
  uint16_t lines_size;
  uint16_t lines_used;
  uint8_t* overlap;
  uint16_t overlap_size;
  uint16_t overlap_used;
} read_lines_state_t;

uint8_t read_lines(int file_descriptor, read_lines_state_t* state) {
  /* behavior
     - read delimiter-terminated lines from file
     - all lines must end with the delimiter
     - .data will contain the lines read from standard input,
     - .lines will contain indexes to the start of each line in .data
     - (overlap_size + data_size / 2) defines the maximum line length
     - in .data, the delimiter will be replaced with 0 to make lines null-separated strings (needed for libc system calls)
     error handling
     - return 0 on success, 1 on read error (errno will be set), 2 on too-long line error
     usage
     - read_lines_state_t state = {0};
     - set delimiter, data, data_size, overlap, overlap_size, lines, line_size
     - read_lines(fd, &state);
     algorithm
     - two buffers: data and overlap
     - read into data
     - search for newline in second half of data
     - if not found then return error
     - copy string after last newline into overlap
     - on next call, copy contents of overlap to data and read at offset */
  ssize_t read_size;
  while (0 < read(0, state->data + state->data_used, read_lines_chunk_size)) {
    if (data_size < data_used + read_size + read_lines_chunk_size) {
      // data too small
    }
    for (read_lines_data_size_t i = 0; i < read_size; i += 1) {
      if (delimiter == (*data)[data_used + i]) {
        if (indexes_size < *indexes_used) {
          // lines too small
        }
        (*data)[data_used + i] = 0;
        (*indexes)[*indexes_used] = data_used + i;
        *indexes_used += 1;
      }
    }
    data_used += read_size;
  }
  if (0 > read_size) errno_error;
}


typedef struct {
  uint8_t delimiter;
  uint8_t* data;
  uint32_t data_size;
  uint32_t data_used;
  uint16_t* lines;
  uint16_t lines_size;
  uint16_t lines_used;
} read_all_lines_state;

uint8_t read_all_lines(read_all_lines_state_t state) {
  // read delimiter-terminated lines from standard input.
  // all lines must end with the delimiter.
  // data will contain the full string read from standard input,
  // while indexes contains indexes to the start of each line in data.
  // the delimiter will be replaced with 0 to make lines null-separated strings (needed for libc system calls).
  // use-case: lines that are all needed later and can therefore not be read in chunks.
  uint32_t data_used = 0;
  uint32_t data_size = lines_initial_data_size;
  uint32_t indexes_size = lines_initial_count;
  ssize_t read_size;
  *data = tmalloc(data_size);
  if (tmalloc_is_failure(*data)) errno_error;
  *indexes = tmalloc(indexes_size * sizeof(uint32_t));
  if (tmalloc_is_failure(*indexes)) errno_error;
  **indexes = 0;
  *indexes_used = 1;
  while (0 < read(0, *data + data_used, lines_initial_data_size / 2)) {
    if (data_size < data_used + read_size + lines_initial_data_size / 2) {
      *data = trealloc(*data, data_size, data_size * data_growth_factor);
      if (tmalloc_is_failure(*data)) errno_error;
      data_size *= data_growth_factor;
    }
    for (uint32_t i = 0; i < read_size; i += 1) {
      if (delimiter == (*data)[data_used + i]) {
        if (indexes_size < *indexes_used) {
          *indexes = trealloc(*indexes, indexes_size * sizeof(uint32_t), indexes_size * data_growth_factor * sizeof(uint32_t));
          if (tmalloc_is_failure(*indexes)) errno_error;
          indexes_size *= data_growth_factor;
        }
        (*data)[data_used + i] = 0;
        (*indexes)[*indexes_used] = data_used + i;
        *indexes_used += 1;
      }
    }
    data_used += read_size;
  }
  if (0 > read_size) errno_error;
  // correct for a trailing newline
  if (delimiter == (*data)[data_used]) *indexes_used -= 1;
}
