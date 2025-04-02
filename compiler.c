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

typedef struct {
  char *name;
  const char *type;
} CompiledExpr;

// this should return the way to get the value out of the expression
CompiledExpr compile_expr(StringBuilder *decl, StringBuilder *impl, Expr expr,
                          int *name) {
  switch (expr.type) {
  case EXPR_INT: {
    char *str = malloc(_scprintf("%d", expr.value._int) + 1);
    sprintf(str, "%d", expr.value._int);
    return (CompiledExpr){.name = str, .type = "i32"};
  }
  case EXPR_FLOAT: {
    char *str = malloc(_scprintf("%f", expr.value._float) + 1);
    sprintf(str, "%f", expr.value._float);
    return (CompiledExpr){.name = str, .type = "f32"};
  }
  case EXPR_OP: {
    Operation op = *expr.value.op;
    switch (op.op) {
    case OP_ADD:
    case OP_SUB:
    case OP_DIV:
    case OP_MUL: {
      CompiledExpr left = compile_expr(decl, impl, op.left, name);
      CompiledExpr right = compile_expr(decl, impl, op.right, name);
      char *nameStr = malloc(12);
      sprintf(nameStr, "%%%d", *name);
      sb_write(impl, nameStr);
      sb_write(impl, " = ");
      switch (op.op) {
      case OP_ADD: {
        sb_write(impl, "add ");
        break;
      }
      case OP_SUB: {
        sb_write(impl, "sub ");
        break;
      }
      case OP_DIV: {
        sb_write(impl, "udiv ");
        break;
      }
      case OP_MUL: {
        sb_write(impl, "mul ");
        break;
      }
      }
      (*name)++;
      sb_write(impl, left.type);
      sb_write(impl, " ");
      sb_write(impl, left.name);
      sb_write(impl, ", ");
      sb_write(impl, right.name);
      sb_write(impl, "\n");
      free(left.name);
      free(right.name);
      return (CompiledExpr){.name = nameStr, .type = left.type};
    }
    }
  }
  case EXPR_BLOCK: {
    CompiledExpr last = {0};
    for (size_t i = 0; i < expr.value.block->stmtc; i++) {
      Expr stmt = expr.value.block->stmts[i];
      CompiledExpr ref = compile_expr(decl, impl, stmt, name);
      if (i >= expr.value.block->stmtc - 1) {
        last = ref;
      } else {
        free(ref.name);
      }
    }
    return last;
  }
  }
  return (CompiledExpr){0};
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
  sb_write(impl, "{\n");

  int varname = 1;
  CompiledExpr var = compile_expr(decl, impl, func.body, &varname);
  sb_write(impl, "ret ");
  sb_write(impl, var.type);
  sb_write(impl, var.name);
  free(var.name);
  sb_write(impl, "}\n");

  free_type(returnType);
}
