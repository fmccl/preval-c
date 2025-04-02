#include "compiler.h"
#include "operator.h"
#include "parser.h"
#include "sb.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "memtracker.h"
#include "type.h"

#ifndef _WIN32
#define _scprintf(format, ...) snprintf(NULL, 0, format, __VA_ARGS__)
#endif

char *type_to_llvm(Type type) {
  switch (type.type) {
  case TYPE_I32: {
    return "i32";
  }
  case TYPE_F32: {
    return "f32";
  }
  }
  return NULL;
}

// this should return the way to get the value out of the expression
char *compile_expr(StringBuilder *decl, StringBuilder *impl, Expr expr,
                   int *name) {
  switch (expr.type) {
  case EXPR_INT: {
    char *str = malloc(_scprintf("i32 %d", expr.value._int) + 1);
    sprintf(str, "i32 %d", expr.value._int);
    return str;
  }
  case EXPR_BLOCK: {
    char *last = NULL;
    for (size_t i = 0; i < expr.value.block->stmtc; i++) {
      Expr stmt = expr.value.block->stmts[i];
      char *ref = compile_expr(decl, impl, stmt, name);
      if (i < expr.value.block->stmtc - 1) {
        last = ref;
      } else {
        free(ref);
      }
    }
    return last;
  }
  }
  return NULL;
}

void compile_function(StringBuilder *decl, StringBuilder *impl, FuncExpr func,
                      char *name) {
  sb_write(impl, "define ");
  Type returnType = infer_type(func.body, NULL, 0);
  sb_write(impl, type_to_llvm(returnType));
  sb_write(impl, " @");
  sb_write(impl, name);
  sb_write(impl, "(");
  for (size_t i = 0; i < func.argc; i++) {
    Type type = parse_type(func.args[i].type);
    sb_write(impl, type_to_llvm(type));
    sb_write(impl, " %");
    sb_write(impl, func.args[i].name);
    sb_write(impl, ",");
    free_type(type);
  }
  sb_write(impl, ")");
  sb_write(impl, "{");

  int varname = 0;
  char *var = compile_expr(decl, impl, func.body, &varname);
  sb_write(impl, "ret ");
  sb_write(impl, var);
  free(var);

  sb_write(impl, "}");

  free_type(returnType);
}
