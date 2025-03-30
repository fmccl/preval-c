#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AllocInfo {
  void *ptr;
  size_t size;
  const char *file;
  int line;
  struct AllocInfo *next;
} AllocInfo;

static AllocInfo *allocList = NULL;

void *debug_malloc(size_t size, const char *file, int line) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Malloc failed at %s:%d\n", file, line);
    return NULL;
  }

  AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo));
  if (!info) {
    fprintf(stderr, "Failed to track allocation at %s:%d\n", file, line);
    free(ptr);
    return NULL;
  }

  info->ptr = ptr;
  info->size = size;
  info->file = file;
  info->line = line;
  info->next = allocList;
  allocList = info;

  return ptr;
}

void *debug_realloc(void *ptr, size_t size, const char *file, int line) {
  if (!ptr) {
    return debug_malloc(size, file, line);
  }

  AllocInfo *prev = NULL, *current = allocList;
  while (current) {
    if (current->ptr == ptr) {
      void *new_ptr = realloc(ptr, size);
      if (!new_ptr) {
        fprintf(stderr, "Realloc failed at %s:%d\n", file, line);
        return NULL;
      }

      current->ptr = new_ptr;
      current->size = size;
      current->file = file;
      current->line = line;
      return new_ptr;
    }
    prev = current;
    current = current->next;
  }

  fprintf(stderr, "Attempted to realloc unknown pointer at %s:%d\n", file,
          line);
  return NULL;
}

void debug_free(void *ptr, const char *file, int line) {
  if (!ptr)
    return;

  AllocInfo *prev = NULL, *current = allocList;
  while (current) {
    if (current->ptr == ptr) {
      if (prev) {
        prev->next = current->next;
      } else {
        allocList = current->next;
      }
      free(current);
      free(ptr);
      return;
    }
    prev = current;
    current = current->next;
  }

  fprintf(stderr, "Attempted to free unknown pointer at %s:%d\n", file, line);
}

void *debug_calloc(size_t count, size_t size, const char *file, int line) {
  void *ptr = calloc(count, size);
  if (!ptr) {
    fprintf(stderr, "Calloc failed at %s:%d\n", file, line);
  }

  AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo));
  if (!info) {
    fprintf(stderr, "Failed to track allocation at %s:%d\n", file, line);
    free(ptr);
    return NULL;
  }

  info->ptr = ptr;
  info->size = count * size;
  info->file = file;
  info->line = line;
  info->next = allocList;
  allocList = info;

  return ptr;
}

void report_leaks(void) {
  AllocInfo *current = allocList;
  if (current) {
    fprintf(stderr, "Memory leaks detected:\n");
    while (current) {
      fprintf(stderr, "Leaked %zu bytes at %s:%d (ptr: %p)\n", current->size,
              current->file, current->line, current->ptr);
      AllocInfo *next = current->next;
      free(current);
      current = next;
    }
    allocList = NULL;
  } else {
    printf("No memory leaks detected.\n");
  }
}