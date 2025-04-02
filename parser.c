#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memtracker.h"
#include "parser.h"
#include "tokeniser.h"

char *parse(Expr *expr, TokenVec outer_tokens, bool shouldFreeTokens) {
  TokenVec tokens = outer_tokens;
  if (tokens.length == 0) {
    return "Can't parse empty tokenvec";
  }

  if (tokens.length == 1 && tokens.tokens[0].type == TT_PARENS) {
    ParensToken callToken = *(ParensToken *)tokens.tokens[0].value.parens;
    if (callToken.argc == 1) {
      tokens = callToken.args[0];
    }
  }

  int lowest_p = 9999;
  int lowest_p_idx = -1;

  for (int i = 0; i < tokens.length; i++) {
    Token token = tokens.tokens[i];

    if (token.type == TT_OP) {
      Operator op = token.value.op;
      int p = precidence(op);
      if (p < lowest_p) {
        lowest_p = p;
        lowest_p_idx = i;
      }
    }
  }

  if (lowest_p_idx != -1) {

    TokenVec left = {0};
    TokenVec right = {0};

    for (int i = 0; i < lowest_p_idx; i++) {
      append_token(&left, copy_token(tokens.tokens[i]));
    }

    for (int i = lowest_p_idx + 1; i < tokens.length; i++) {
      append_token(&right, copy_token(tokens.tokens[i]));
    }

    if ((tokens.tokens[lowest_p_idx].value.op) == OP_ARROW) {
      *expr = (Expr){.type = EXPR_FUNC,
                     .value.func = (void *)malloc(sizeof(FuncExpr))};

      Expr rightExpr = {.type = EXPR_NULL, .value = NULL};

      char *error = parse(&rightExpr, right, true);
      if (error) {
        return error;
      }

      int argc = 0;
      if (left.length > 1) {
        return "Can't parse function with more than one argument list";
      }
      if (left.tokens[0].type == TT_PARENS) {
        TokenVec *argTokens =
            ((ParensToken *)left.tokens[0].value.parens)->args;

        argc = ((ParensToken *)left.tokens[0].value.parens)->argc;

        *(FuncExpr *)(expr->value.func) = (FuncExpr){
            .args = calloc(argc, sizeof(Arg)), .argc = argc, .body = rightExpr};

        for (int i = 0; i < argc; i++) {
          if (argTokens[i].length == 2) {
            return "Can't parse function with non-name[: type] argument";
          }
          if (argTokens[i].length == 1) {
            Token name = argTokens[i].tokens[0];
            if (name.type == TT_NAME) {
              FuncExpr *funcExpr = (FuncExpr *)expr->value.func;
              char *nameStr = malloc(strlen(name.value.name) + 1);
              strcpy(nameStr, name.value.name);
              funcExpr->args[i] = (Arg){.name = nameStr, .type = NULL};
            } else {
              return "Can't use an expression as a function parameter name";
            }
          } else {
            Token name = argTokens[i].tokens[0];
            Token colon = argTokens[i].tokens[1];
            TokenVec type = {0};
            for (int j = 2; j < argTokens[i].length; j++) {
              append_token(&type, copy_token(argTokens[i].tokens[j]));
            }
            if (name.type == TT_NAME && colon.type == TT_COLON) {
              Expr typeExpr = {.type = EXPR_NULL, .value = NULL};
              char *error = parse(&typeExpr, type, true);
              if (error) {
                return error;
              }

              if (typeExpr.type != EXPR_NAME) {
                return "Types must be names";
              }
              char *typeName = typeExpr.value.name;

              FuncExpr *funcExpr = (FuncExpr *)(expr->value.func);
              char *nameStr = malloc(strlen(name.value.name) + 1);
              strcpy(nameStr, name.value.name);
              funcExpr->args[i] = (Arg){.name = nameStr, .type = typeName};
            } else {
              for (int j = 0; j < argTokens[i].length; j++) {
                print_token(argTokens[i].tokens[j]);
              }
              return "Can't parse function with non-name: type argument";
            }
          }
        }
      } else {
        return "Can't parse function with non-argument argument list";
      }

      for (int i = 0; i < left.length; i++) {
        free_token(left.tokens[i]);
      }
      free(left.tokens);
    } else {

      *expr = (Expr){.type = EXPR_OP,
                     .value.op = (void *)malloc(sizeof(Operation))};

      Expr leftExpr = {.type = EXPR_NULL, .value = NULL};
      Expr rightExpr = {.type = EXPR_NULL, .value = NULL};

      char *error = parse(&leftExpr, left, true);
      if (error) {
        return error;
      }

      error = parse(&rightExpr, right, true);
      if (error) {
        return error;
      }

      *(Operation *)(expr->value.op) =
          (Operation){.left = leftExpr,
                      .right = rightExpr,
                      .op = tokens.tokens[lowest_p_idx].value.op};
    }
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_INT) {
    *expr = (Expr){.type = EXPR_INT, .value._int = tokens.tokens[0].value._int};
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_FLOAT) {
    *expr = (Expr){.type = EXPR_FLOAT,
                   .value._float = tokens.tokens[0].value._float};
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_NAME) {
    *expr = (Expr){.type = EXPR_NAME, .value.name = malloc(sizeof(char *))};
    strcpy((char *)expr->value.name, tokens.tokens[0].value.name);
  } else if (tokens.tokens[tokens.length - 1].type == TT_PARENS) {
    ParensToken ct =
        *(ParensToken *)tokens.tokens[tokens.length - 1].value.parens;
    *expr = (Expr){.type = EXPR_CALL,
                   .value.call = (void *)malloc(sizeof(CallExpr))};

    Expr *args = malloc(sizeof(Expr) * ct.argc);
    for (int i = 0; i < ct.argc; i++) {
      Expr arg = {.type = EXPR_NULL, .value = NULL};
      char *error = parse(&arg, ct.args[i], false);
      if (error) {
        return error;
      }
      args[i] = arg;
    }

    Expr func = {.type = EXPR_NULL, .value = NULL};
    // get all tokens to the left of the call
    TokenVec left = {0};
    for (int i = 0; i < tokens.length - 1; i++) {
      append_token(&left, copy_token(tokens.tokens[i]));
    }
    char *error = parse(&func, left, true);
    if (error) {
      return error;
    }

    *(CallExpr *)(expr->value.call) =
        (CallExpr){.func = func, .args = args, .argc = ct.argc};
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_BLOCK) {
    BlockToken bt = *(BlockToken *)tokens.tokens[0].value.block;

    *expr = (Expr){.type = EXPR_BLOCK,
                   .value.block = (void *)malloc(sizeof(BlockExpr))};

    Expr *stmts = malloc(sizeof(Expr) * bt.stmtc);

    for (int i = 0; i < bt.stmtc; i++) {
      Expr stmt = {.type = EXPR_NULL, .value = NULL};
      char *error = parse(&stmt, bt.stmts[i], false);
      if (error) {
        return error;
      }
      stmts[i] = stmt;
    }

    *(BlockExpr *)(expr->value.block) =
        (BlockExpr){.stmts = stmts, .stmtc = bt.stmtc, .returns = bt.returns};
  } else {
    return "Can't parse tokenvec";
  }

  if (shouldFreeTokens) {
    for (int i = 0; i < outer_tokens.length; i++) {
      free_token(outer_tokens.tokens[i]);
    }
    free(outer_tokens.tokens);
  }
  return NULL;
}

void free_expr(Expr expr) {
  if (expr.type == EXPR_OP) {
    Operation op = *(Operation *)expr.value.op;
    free_expr(op.left);
    free_expr(op.right);
    free(expr.value.op);
  }
  if (expr.type == EXPR_CALL) {
    CallExpr call = *(CallExpr *)expr.value.call;
    for (int i = 0; i < call.argc; i++) {
      free_expr(call.args[i]);
    }
    free_expr(call.func);
    free(call.args);
    free(expr.value.call);
  }
  if (expr.type == EXPR_BLOCK) {
    BlockExpr block = *(BlockExpr *)expr.value.block;
    for (int i = 0; i < block.stmtc; i++) {
      free_expr(block.stmts[i]);
    }
    free(block.stmts);
    free(expr.value.block);
  }
  if (expr.type == EXPR_FUNC) {
    FuncExpr func = *(FuncExpr *)expr.value.func;
    for (int i = 0; i < func.argc; i++) {
      free(func.args[i].type);
    }
    free(func.args);
    free_expr(func.body);
    free(expr.value.func);
  }
  if (expr.type == EXPR_NAME) {
    free(expr.value.name);
  }
}

#define parse(expr, tokens) parse(expr, tokens, true)

void print_expr(Expr expr) {
  if (expr.type == EXPR_INT) {
    printf("%d", expr.value._int);
  } else if (expr.type == EXPR_FLOAT) {
    printf("%ff", expr.value._float);
  } else if (expr.type == EXPR_OP) {
    Operation op = *(Operation *)expr.value.op;
    printf("(");
    print_expr(op.left);
    printf(" %c ", op.op);
    print_expr(op.right);
    printf(")");
  } else if (expr.type == EXPR_NULL) {
    printf("NULL");
  } else if (expr.type == EXPR_NAME) {
    printf("%s", (char *)expr.value.name);
  } else if (expr.type == EXPR_CALL) {
    CallExpr call = *(CallExpr *)expr.value.call;
    print_expr(call.func);
    printf("(");
    for (int i = 0; i < call.argc; i++) {
      print_expr(call.args[i]);
      if (i < call.argc - 1) {
        printf(", ");
      }
    }
    printf(")");
  } else if (expr.type == EXPR_BLOCK) {
    BlockExpr block = *(BlockExpr *)expr.value.block;
    printf("{\n");
    for (int i = 0; i < block.stmtc; i++) {
      print_expr(block.stmts[i]);
      printf(";\n");
    }
    printf("}");
  } else if (expr.type == EXPR_FUNC) {
    FuncExpr func = *(FuncExpr *)expr.value.func;
    printf("(");
    for (int i = 0; i < func.argc; i++) {
      printf("%s", func.args[i].name);
      printf(":");
      printf("%s", func.args[i].type);
      if (i < func.argc - 1) {
        printf(", ");
      }
    }
    printf(") => ");
    print_expr(func.body);
  }
}