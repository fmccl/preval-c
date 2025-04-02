#ifndef TYPE_H
#define TYPE_H

#include "parser.h"
#include <stddef.h>

typedef struct Type Type;

struct Type {
  enum { TYPE_NULL, TYPE_I32, TYPE_F32, TYPE_FUNC } type;
  union {
    struct FuncType {
      struct Type *returnType;
      struct Type *args;
      size_t argc;
    } funcType;
  } value;
};

typedef struct {
  char *name;
  Type type;
} Name;

Type infer_type(Expr expr, Name *names, size_t namec);

void free_type(Type type);

Type parse_type(char *in);

#endif