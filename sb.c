#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "memtracker.h"
#include "sb.h"

#include <stddef.h>
#include <string.h>

void sb_write(StringBuilder *sb, const char *text) {
  if (sb->length >= sb->capacity) {
    sb->capacity = 1 + sb->capacity * 2;
    sb->strings = realloc(sb->strings, sizeof(char *) * sb->capacity);
  }
  char *str = malloc(strlen(text) + 1);
  strcpy(str, text);
  sb->strings[sb->length++] = str;
}

char *sb_to_string(StringBuilder *sb, bool free) {
  size_t totalLength = 0;
  for (size_t i = 0; i < sb->length; i++) {
    totalLength += strlen(sb->strings[i]);
  }
  char *out = malloc(sizeof(char) * totalLength + 1);
  size_t idx = 0;
  for (size_t i = 0; i < sb->length; i++) {
    strcpy(out + idx, sb->strings[i]); // Properly copy strings
    idx += strlen(sb->strings[i]);
  }
  out[totalLength] = '\0';
  if (free) {
    for (size_t i = 0; i < sb->length; i++) {
      free(sb->strings[i]);
    }
    free(sb->strings);
  }
  return out;
}