#ifndef PARSER_H
#define PARSER_H
#include "operator.h"
#include "tokeniser.h"
#include <stdbool.h>

typedef struct Expr Expr;
typedef struct Arg Arg;
typedef struct FuncExpr FuncExpr;
typedef struct Operation Operation;
typedef struct CallExpr CallExpr;
typedef struct BlockExpr BlockExpr;

struct Expr {
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
  union {
    int _int;
    char *name;
    float _float;
    Operation *op;
    CallExpr *call;
    BlockExpr *block;
    FuncExpr *func;
  } value;
};

struct Arg {
  char *name;
  char *type;
};

struct FuncExpr {
  Arg *args;
  int argc;
  Expr body;
};

struct Operation {
  Expr left;
  Operator op;
  Expr right;
};

struct CallExpr {
  Expr *args;
  int argc;
  Expr func;
};

struct BlockExpr {
  Expr *stmts;
  int stmtc;
  bool returns;
};

char *parse(Expr *expr, TokenVec outer_tokens, bool shouldFreeTokens);

void free_expr(Expr expr);

void print_expr(Expr expr);

#endif