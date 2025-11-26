char* errno_message;
uint8_t errno_message_len;

#define print_errno() do { \
    errno_message = strerror(errno); \
    write(2, errno_message, strlen(errno_message)); \
    write(2, "\n", 1); \
  } while(0)
