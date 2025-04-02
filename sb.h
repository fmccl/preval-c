#ifndef SB_H
#define SB_H
#include <stdbool.h>
#include <stddef.h>
typedef struct {
  char **strings;
  size_t length;
  size_t capacity;
} StringBuilder;

void sb_write(StringBuilder *, char *);

char *sb_to_string(StringBuilder *, bool);

#endif