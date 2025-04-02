#ifndef SB_H
#define SB_H
#include <stdbool.h>
#include <stddef.h>
typedef struct {
  char **strings;
  size_t length;
  size_t capacity;
} StringBuilder;

void sb_write(StringBuilder *, const char *);

char *sb_to_string(StringBuilder *, bool free);

#endif