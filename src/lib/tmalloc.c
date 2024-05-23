#define tmalloc(size) mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
#define trealloc(pointer, old_size, new_size) mremap(pointer, old_size, new_size, MREMAP_MAYMOVE)
#define tmalloc_is_failure(pointer) MAP_FAILED == pointer
