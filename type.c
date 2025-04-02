#include "type.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "memtracker.h"

Type parse_type(char *name) {
  if (strcmp(name, "i32")) {
    return (Type){.type = TYPE_I32};
  } else if (strcmp(name, "f32")) {
    return (Type){.type = TYPE_F32};
  }

  return (Type){.type = TYPE_NULL};
}

Type infer_type(Expr expr, Name *names, size_t namec) {
  switch (expr.type) {
  case EXPR_OP: // TEMP!!
    return infer_type(expr.value.op->left, names, namec);
  case EXPR_CALL:
    return infer_type(expr.value.call->func.value.func->body, names, namec);
  case EXPR_FUNC: {
    struct Type *rt = malloc(sizeof(Type));
    Type *arg_types = malloc(sizeof(Type) * expr.value.func->argc);
    for (size_t i = 0; i < expr.value.func->argc; i++) {
      arg_types[i] = parse_type(expr.value.func->args[i].type);
    }
    return (Type){.type = TYPE_FUNC,
                  .value.funcType = {
                      .returnType = rt,
                  }};
  }
  case EXPR_NULL: {
    return (Type){.type = TYPE_NULL};
  }
  case EXPR_INT:
    return (Type){.type = TYPE_I32};

  case EXPR_FLOAT:
    return (Type){.type = TYPE_F32};
  case EXPR_NAME: {
    for (size_t i = 0; i < namec; i++) {
      if (names[i].name == expr.value.name) {
        return names[i].type;
      }
    }
    return (Type){.type = TYPE_NULL};
  }
  case EXPR_BLOCK: {
    if (expr.value.block->returns) {
      return infer_type(expr.value.block->stmts[expr.value.block->stmtc - 1],
                        names, namec);
    }
  }
  }
  return (Type){.type = TYPE_NULL};
}

void free_type(Type type) {
  switch (type.type) {
  case TYPE_FUNC: {
    free_type(*type.value.funcType.returnType);
    for (size_t i = 0; i < type.value.funcType.argc; i++) {
      free_type(type.value.funcType.args[i]);
    }
    free(type.value.funcType.args);
    free(type.value.funcType.args);
  }
  }
}