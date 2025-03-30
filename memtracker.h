void *debug_malloc(size_t size, const char *file, int line);

void *debug_realloc(void *ptr, size_t size, const char *file, int line);

void *debug_calloc(size_t count, size_t size, const char *file, int line);

void debug_free(void *ptr, const char *file, int line);

void report_leaks(void);

#define malloc(size) debug_malloc(size, __FILE__, __LINE__)
#define realloc(ptr, size) debug_realloc(ptr, size, __FILE__, __LINE__)
#define calloc(ptr, size) debug_calloc(ptr, size, __FILE__, __LINE__)
#define free(ptr) debug_free(ptr, __FILE__, __LINE__)