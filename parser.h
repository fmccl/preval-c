#ifndef PARSER_H
#define PARSER_H
#include "operator.h"
#include "tokeniser.h"
#include <stdbool.h>

typedef struct {
  enum {
    EXPR_NULL,
    EXPR_INT,   // value *int
    EXPR_NAME,  // value *char
    EXPR_FLOAT, // value *float
    EXPR_OP,    // value *Operation
    EXPR_CALL,  // value *CallExpr
    EXPR_BLOCK, // value *BlockExpr
    EXPR_FUNC   // value *FuncExpr
  } type;
  void *value;
} Expr;

typedef struct {
  char *name;
  Expr type;
} Arg;

typedef struct {
  Arg *args;
  int argc;
  Expr body;
} FuncExpr;

typedef struct {
  Expr left;
  Operator op;
  Expr right;
} Operation;

typedef struct {
  Expr *args;
  int argc;
  Expr func;
} CallExpr;

typedef struct {
  Expr *stmts;
  int stmtc;
  bool returns;
} BlockExpr;

char *parse(Expr *expr, TokenVec outer_tokens, bool shouldFreeTokens);

void free_expr(Expr expr);

void print_expr(Expr expr);

#endif