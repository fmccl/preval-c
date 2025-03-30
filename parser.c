#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memtracker.h"
#include "parser.h"


char *parse(Expr *expr, TokenVec outer_tokens, bool shouldFreeTokens) {
  TokenVec tokens = outer_tokens;
  if (tokens.length == 0) {
    return "Can't parse empty tokenvec";
  }

  if (tokens.length == 1 && tokens.tokens[0].type == TT_PARENS) {
    CallToken callToken = *(CallToken *)tokens.tokens[0].value;
    if (callToken.argc == 1) {
      tokens = callToken.args[0];
    }
  }

  int lowest_p = 9999;
  int lowest_p_idx = -1;

  for (int i = 0; i < tokens.length; i++) {
    Token token = tokens.tokens[i];

    if (token.type == TT_OP) {
      Operator op = *(Operator *)token.value;
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

    if (*(Operator *)(tokens.tokens[lowest_p_idx].value) == OP_ARROW) {
      *expr =
          (Expr){.type = EXPR_FUNC, .value = (void *)malloc(sizeof(FuncExpr))};

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
        TokenVec *argTokens = ((CallToken *)left.tokens[0].value)->args;

        argc = ((CallToken *)left.tokens[0].value)->argc;

        *(FuncExpr *)(expr->value) = (FuncExpr){
            .args = calloc(argc, sizeof(Arg)), .argc = argc, .body = rightExpr};

        for (int i = 0; i < argc; i++) {
          if (argTokens[i].length < 3) {
            return "Can't parse function with non-name: type argument";
          }
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
            FuncExpr *funcExpr = (FuncExpr *)(expr->value);
            funcExpr->args[i] =
                (Arg){.name = _strdup((char *)name.value), .type = typeExpr};
          } else {
            for (int j = 0; j < argTokens[i].length; j++) {
              print_token(argTokens[i].tokens[j]);
            }
            return "Can't parse function with non-name: type argument";
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

      *expr =
          (Expr){.type = EXPR_OP, .value = (void *)malloc(sizeof(Operation))};

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

      *(Operation *)(expr->value) =
          (Operation){.left = leftExpr,
                      .right = rightExpr,
                      .op = *(Operator *)tokens.tokens[lowest_p_idx].value};
    }
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_INT) {
    *expr = (Expr){.type = EXPR_INT, .value = (void *)malloc(sizeof(int))};
    *(int *)expr->value = *(int *)tokens.tokens[0].value;
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_FLOAT) {
    *expr = (Expr){.type = EXPR_FLOAT, .value = (void *)malloc(sizeof(float))};
    *(float *)expr->value = *(float *)tokens.tokens[0].value;
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_NAME) {
    *expr = (Expr){.type = EXPR_NAME, .value = malloc(sizeof(char *))};
    strcpy_s((char *)expr->value, strlen((char *)tokens.tokens[0].value) + 1,
             (char *)tokens.tokens[0].value);
  } else if (tokens.tokens[tokens.length - 1].type == TT_PARENS) {
    CallToken ct = *(CallToken *)tokens.tokens[tokens.length - 1].value;
    *expr =
        (Expr){.type = EXPR_CALL, .value = (void *)malloc(sizeof(CallExpr))};

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

    *(CallExpr *)(expr->value) =
        (CallExpr){.func = func, .args = args, .argc = ct.argc};
  } else if (tokens.length == 1 && tokens.tokens[0].type == TT_BLOCK) {
    BlockToken bt = *(BlockToken *)tokens.tokens[0].value;

    *expr =
        (Expr){.type = EXPR_BLOCK, .value = (void *)malloc(sizeof(BlockExpr))};

    Expr *stmts = malloc(sizeof(Expr) * bt.stmtc);

    for (int i = 0; i < bt.stmtc; i++) {
      Expr stmt = {.type = EXPR_NULL, .value = NULL};
      char *error = parse(&stmt, bt.stmts[i], false);
      if (error) {
        return error;
      }
      stmts[i] = stmt;
    }

    *(BlockExpr *)(expr->value) =
        (BlockExpr){.stmts = stmts, .stmtc = bt.stmtc, .returns = bt.returns};
  } else {
    char error[1024];
    for (int i = 0; i < tokens.length; i++) {
      // print_token(tokens.tokens[i]);
    }
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
    Operation op = *(Operation *)expr.value;
    free_expr(op.left);
    free_expr(op.right);
  }
  if (expr.type == EXPR_CALL) {
    CallExpr call = *(CallExpr *)expr.value;
    for (int i = 0; i < call.argc; i++) {
      free_expr(call.args[i]);
    }
    free_expr(call.func);
    free(call.args);
  }
  if (expr.type == EXPR_BLOCK) {
    BlockExpr block = *(BlockExpr *)expr.value;
    for (int i = 0; i < block.stmtc; i++) {
      free_expr(block.stmts[i]);
    }
    free(block.stmts);
  }
  if (expr.type == EXPR_FUNC) {
    FuncExpr func = *(FuncExpr *)expr.value;
    for (int i = 0; i < func.argc; i++) {
      free_expr(func.args[i].type);
    }
    free(func.args);
    free_expr(func.body);
  }
  free(expr.value);
}

#define parse(expr, tokens) parse(expr, tokens, true)

void print_expr(Expr expr) {
  if (expr.type == EXPR_INT) {
    printf("%d", *(int *)expr.value);
  } else if (expr.type == EXPR_FLOAT) {
    printf("%f", *(float *)expr.value);
  } else if (expr.type == EXPR_OP) {
    Operation op = *(Operation *)expr.value;
    printf("(");
    print_expr(op.left);
    printf(" %c ", op.op);
    print_expr(op.right);
    printf(")");
  } else if (expr.type == EXPR_NULL) {
    printf("NULL");
  } else if (expr.type == EXPR_NAME) {
    printf("%s", (char *)expr.value);
  } else if (expr.type == EXPR_CALL) {
    CallExpr call = *(CallExpr *)expr.value;
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
    BlockExpr block = *(BlockExpr *)expr.value;
    printf("{\n");
    for (int i = 0; i < block.stmtc; i++) {
      print_expr(block.stmts[i]);
      printf(";\n");
    }
    printf("}");
  } else if (expr.type == EXPR_FUNC) {
    FuncExpr func = *(FuncExpr *)expr.value;
    printf("(");
    for (int i = 0; i < func.argc; i++) {
      printf("%s", func.args[i].name);
      printf(":");
      print_expr(func.args[i].type);
      if (i < func.argc - 1) {
        printf(", ");
      }
    }
    printf(") => ");
    print_expr(func.body);
  }
}