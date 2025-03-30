#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AllocInfo {
  void *ptr;
  size_t size;
  const char *file;
  int line;
  struct AllocInfo *next;
} AllocInfo;

static AllocInfo *allocList = NULL;

void *debug_malloc(size_t size, const char *file, int line) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Malloc failed at %s:%d\n", file, line);
    return NULL;
  }

  AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo));
  if (!info) {
    fprintf(stderr, "Failed to track allocation at %s:%d\n", file, line);
    free(ptr);
    return NULL;
  }

  info->ptr = ptr;
  info->size = size;
  info->file = file;
  info->line = line;
  info->next = allocList;
  allocList = info;

  return ptr;
}

void *debug_realloc(void *ptr, size_t size, const char *file, int line) {
  if (!ptr) {
    return debug_malloc(size, file, line);
  }

  AllocInfo *prev = NULL, *current = allocList;
  while (current) {
    if (current->ptr == ptr) {
      void *new_ptr = realloc(ptr, size);
      if (!new_ptr) {
        fprintf(stderr, "Realloc failed at %s:%d\n", file, line);
        return NULL;
      }

      current->ptr = new_ptr;
      current->size = size;
      current->file = file;
      current->line = line;
      return new_ptr;
    }
    prev = current;
    current = current->next;
  }

  fprintf(stderr, "Attempted to realloc unknown pointer at %s:%d\n", file,
          line);
  return NULL;
}

void debug_free(void *ptr, const char *file, int line) {
  if (!ptr)
    return;

  AllocInfo *prev = NULL, *current = allocList;
  while (current) {
    if (current->ptr == ptr) {
      if (prev) {
        prev->next = current->next;
      } else {
        allocList = current->next;
      }
      free(current);
      free(ptr);
      return;
    }
    prev = current;
    current = current->next;
  }

  fprintf(stderr, "Attempted to free unknown pointer at %s:%d\n", file, line);
}

void *debug_calloc(size_t count, size_t size, const char *file, int line) {
  void *ptr = calloc(count, size);
  if (!ptr) {
    fprintf(stderr, "Calloc failed at %s:%d\n", file, line);
  }

  AllocInfo *info = (AllocInfo *)malloc(sizeof(AllocInfo));
  if (!info) {
    fprintf(stderr, "Failed to track allocation at %s:%d\n", file, line);
    free(ptr);
    return NULL;
  }

  info->ptr = ptr;
  info->size = count * size;
  info->file = file;
  info->line = line;
  info->next = allocList;
  allocList = info;

  return ptr;
}

void report_leaks(void) {
  AllocInfo *current = allocList;
  if (current) {
    fprintf(stderr, "Memory leaks detected:\n");
    while (current) {
      fprintf(stderr, "Leaked %zu bytes at %s:%d (ptr: %p)\n", current->size,
              current->file, current->line, current->ptr);
      AllocInfo *next = current->next;
      free(current);
      current = next;
    }
    allocList = NULL;
  } else {
    printf("No memory leaks detected.\n");
  }
}

#define malloc(size) debug_malloc(size, __FILE__, __LINE__)
#define realloc(ptr, size) debug_realloc(ptr, size, __FILE__, __LINE__)
#define calloc(ptr, size) debug_calloc(ptr, size, __FILE__, __LINE__)
#define free(ptr) debug_free(ptr, __FILE__, __LINE__)

#include <corecrt.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
  enum {
    TT_INT,    // value *int
    TT_FLOAT,  // value *float
    TT_OP,     // value *Operator
    TT_NAME,   // value *char
    TT_PARENS, // value *CallToken
    TT_BLOCK,  // value *TokenVec
    TT_COLON,
  } type;
  void *value;
} Token;

typedef struct {
  int capacity;
  int length;
  Token *tokens;
} TokenVec;

typedef struct {
  TokenVec *args;
  int argc;
} CallToken;

typedef struct {
  TokenVec *stmts;
  int stmtc;
  bool returns;
} BlockToken;

void append_token(TokenVec *vec, Token token) {
  if (vec->capacity == vec->length) {
    vec->capacity += 1;
    vec->capacity *= 2;
    vec->tokens = realloc(vec->tokens, vec->capacity * sizeof(Token));
  }
  vec->tokens[vec->length] = token;
  vec->length++;
}

typedef enum {
  OP_ADD = '+',
  OP_SUB = '-',
  OP_MUL = '*',
  OP_DIV = '/',
  OP_ASSIGN = '=',
  OP_ARROW,
} Operator;

int precidence(Operator op) {
  switch (op) {
  case OP_ADD:
    return 1;
  case OP_SUB:
    return 1;
  case OP_MUL:
    return 2;
  case OP_DIV:
    return 2;
  case OP_ASSIGN:
    return 0;
  case OP_ARROW:
    return 0;
  }
}

Token copy_token(Token token) {
  Token newToken = {0};
  newToken.type = token.type;
  switch (token.type) {
  case TT_COLON:
    newToken.type = TT_COLON;
    break;
  case TT_FLOAT:
    newToken.value = malloc(sizeof(float));
    *(float *)newToken.value = *(float *)token.value;
    break;
  case TT_INT:
    newToken.value = malloc(sizeof(int));
    *(int *)newToken.value = *(int *)token.value;
    break;
  case TT_OP:
    newToken.value = malloc(sizeof(Operator));
    *(Operator *)newToken.value = *(Operator *)token.value;
    break;
  case TT_NAME:
    newToken.value = malloc(strlen((char *)token.value) + 1);
    strcpy_s((char *)newToken.value, strlen((char *)token.value) + 1,
             (char *)token.value);
    break;
  case TT_PARENS:
    TokenVec *newTokenVecs =
        malloc(sizeof(TokenVec) * ((CallToken *)token.value)->argc);

    for (int i = 0; i < ((CallToken *)token.value)->argc; i++) {
      newTokenVecs[i] = (TokenVec){0};
      for (int j = 0; j < ((CallToken *)token.value)->args[i].length; j++) {
        append_token(&newTokenVecs[i],
                     copy_token(((CallToken *)token.value)->args[i].tokens[j]));
      }
    }

    newToken.value = malloc(sizeof(CallToken));
    ((CallToken *)newToken.value)->args = newTokenVecs;
    ((CallToken *)newToken.value)->argc = ((CallToken *)token.value)->argc;
    break;
  case TT_BLOCK: {
    TokenVec *newTokenVecs =
        malloc(sizeof(TokenVec) * ((BlockToken *)token.value)->stmtc);

    for (int i = 0; i < ((BlockToken *)token.value)->stmtc; i++) {
      newTokenVecs[i] = (TokenVec){0};
      for (int j = 0; j < ((BlockToken *)token.value)->stmts[i].length; j++) {
        append_token(
            &newTokenVecs[i],
            copy_token(((BlockToken *)token.value)->stmts[i].tokens[j]));
      }
    }

    newToken.value = malloc(sizeof(BlockToken));
    ((BlockToken *)newToken.value)->stmts = newTokenVecs;
    ((BlockToken *)newToken.value)->stmtc = ((BlockToken *)token.value)->stmtc;
  }
  }
  return newToken;
}

void print_token(Token token) {
  switch (token.type) {
  case TT_FLOAT:
    printf("%f", *(float *)token.value);
    break;
  case TT_INT:
    printf("%d", *(int *)token.value);
    break;
  case TT_OP:
    printf("%c", *(Operator *)token.value);
    break;
  case TT_NAME:
    printf("%s", (char *)token.value);
    break;
  case TT_COLON:
    printf(":");
    break;
  case TT_PARENS:
    printf("(");
    CallToken *callToken = (CallToken *)token.value;
    for (int i = 0; i < callToken->argc; i++) {
      for (int j = 0; j < callToken->args[i].length; j++) {
        print_token(callToken->args[i].tokens[j]);
      }
      printf(", ");
    }
    printf(")");
    break;
  case TT_BLOCK:
    printf("{");
    BlockToken *blockToken = (BlockToken *)token.value;
    for (int i = 0; i < blockToken->stmtc; i++) {
      for (int j = 0; j < blockToken->stmts[i].length; j++) {
        print_token(blockToken->stmts[i].tokens[j]);
      }
      printf("\n");
    }
    printf("}");
    break;
  }
}

void free_token(Token token) {
  if (token.type == TT_PARENS) {
    CallToken *callToken = (CallToken *)token.value;
    for (int i = 0; i < callToken->argc; i++) {
      for (int j = 0; j < callToken->args[i].length; j++) {
        free_token(callToken->args[i].tokens[j]);
      }
      free(callToken->args[i].tokens);
    }
    free(callToken->args);
  }
  if (token.type == TT_BLOCK) {
    BlockToken *blockToken = (BlockToken *)token.value;
    for (int i = 0; i < blockToken->stmtc; i++) {
      for (int j = 0; j < blockToken->stmts[i].length; j++) {
        free_token(blockToken->stmts[i].tokens[j]);
      }
      free(blockToken->stmts[i].tokens);
    }
    free(blockToken->stmts);
  }
  free(token.value);
}

TokenVec tokenize(char *buf, int len) {

  TokenVec vec = {0};
  int i = 0;

  while (i < len) {
    if (isspace(buf[i])) {
      i++;
      continue;
    } else if (isdigit(buf[i]) || buf[i] == '.') {
      bool decimal = false;
      int numLen = 0;
      while (i + numLen < len && (isdigit(buf[i + numLen]) ||
                                  (!decimal && buf[i + numLen] == '.'))) {
        if (buf[i + numLen] == '.') {
          decimal = true;
        }
        numLen++;
      }
      char *numStr = malloc(numLen + 1);
      memcpy(numStr, buf + i, numLen);
      i += numLen;
      numStr[numLen] = '\0';

      Token token = {0};
      if (decimal) {
        token.type = TT_FLOAT;
        token.value = malloc(sizeof(float));
        *(float *)token.value = atof(numStr);
      } else {
        token.type = TT_INT;
        token.value = malloc(sizeof(int));
        *(int *)token.value = atoi(numStr);
      }
      append_token(&vec, token);
      free(numStr);
    } else if (buf[i] == '+') {
      Operator *op = malloc(sizeof(Operator));
      *op = OP_ADD;
      Token token = {.type = TT_OP, .value = op};
      append_token(&vec, token);
      i++;
    } else if (buf[i] == ':') {
      Token token = {.type = TT_COLON};
      append_token(&vec, token);
      i++;
    } else if (buf[i] == '=') {
      Operator *op = malloc(sizeof(Operator));
      if (i + 1 < len && buf[i + 1] == '>') {
        i++;
        *op = OP_ARROW;
      } else {
        *op = OP_ASSIGN;
      }

      Token token = {.type = TT_OP, .value = op};
      append_token(&vec, token);
      i++;
    } else if (buf[i] == '-') {
      Operator *op = malloc(sizeof(Operator));
      *op = OP_SUB;
      Token token = {.type = TT_OP, .value = op};
      append_token(&vec, token);
      i++;
    } else if (buf[i] == '/') {
      Operator *op = malloc(sizeof(Operator));
      *op = OP_DIV;
      Token token = {.type = TT_OP, .value = op};
      append_token(&vec, token);
      i++;
    } else if (buf[i] == '*') {
      Operator *op = malloc(sizeof(Operator));
      *op = OP_MUL;
      Token token = {.type = TT_OP, .value = op};
      append_token(&vec, token);
      i++;
    } else if (isalnum(buf[i]) || buf[i] == '_') {
      int nameLen = 0;
      while (i + nameLen < len &&
             (isalnum(buf[i + nameLen]) || buf[i + nameLen] == '_')) {
        nameLen++;
      }
      char *name = malloc(nameLen + 1);
      memcpy(name, buf + i, nameLen);
      name[nameLen] = '\0';
      i += nameLen;

      Token token = {.type = TT_NAME, .value = name};
      append_token(&vec, token);
    } else if (buf[i] == '(') {
      i++;
      int argc = 0;
      int open = 1;
      int contentsLen = 0;
      bool foundNonWhitespace = false;
      for (int j = i; j < len; j++) {
        contentsLen++;
        if (!(open == 1 && buf[j] == ')') && !foundNonWhitespace &&
            !isspace(buf[j])) {
          argc++;
          foundNonWhitespace = true;
        }
        if (buf[j] == '(') {
          open++;
        }
        if (buf[j] == ')') {
          open--;
          if (open == 0) {
            contentsLen--;
            break;
          }
        }
        if (open == 1 && buf[j] == ',') {
          argc++;
        }
      }

      TokenVec *args = calloc(argc, sizeof(TokenVec));
      int argIdx = 0;

      int *argLengths = calloc(argc, sizeof(int));
      open = 1;

      for (int j = i; j < i + contentsLen; j++) {
        if (buf[j] == '(') {
          open++;
        }
        if (buf[j] == ')') {
          open--;
          if (open == 0) {
            break;
          }
        }
        if (open == 1 && buf[j] == ',') {
          argIdx++;
        } else {
          argLengths[argIdx]++;
        }
      }

      char **strArgs = calloc(argc, sizeof(char *));

      for (int j = 0; j < argc; j++) {
        strArgs[j] = malloc(argLengths[j] + 1);
        memcpy(strArgs[j], buf + i, argLengths[j]);
        strArgs[j][argLengths[j]] = '\0';
        i += argLengths[j] + 1; // +1 for the comma
      }

      for (int j = 0; j < argc; j++) {
        args[j] = tokenize(strArgs[j], argLengths[j]);
        free(strArgs[j]);
      }

      free(argLengths);
      free(strArgs);

      Token token = {0};
      token.type = TT_PARENS;
      token.value = malloc(sizeof(CallToken));
      ((CallToken *)token.value)->args = args;
      ((CallToken *)token.value)->argc = argc;
      append_token(&vec, token);
    } else if (buf[i] == '{') {
      i++;
      int argc = 0;
      int open = 1;
      int contentsLen = 0;
      bool foundNonWhitespace = false;
      for (int j = i; j < len; j++) {
        contentsLen++;
        if (!(open == 1 && buf[j] == '}') && !foundNonWhitespace &&
            !isspace(buf[j])) {
          argc++;
          foundNonWhitespace = true;
        }
        if (buf[j] == '{') {
          open++;
        }
        if (buf[j] == '}') {
          open--;
          if (open == 0) {
            contentsLen--;
            break;
          }
        }
        if (open == 1 && buf[j] == ';') {
          argc++;
        }
      }

      TokenVec *args = calloc(argc, sizeof(TokenVec));
      int argIdx = 0;

      int *argLengths = calloc(argc, sizeof(int));
      open = 1;

      for (int j = i; j < i + contentsLen; j++) {
        if (buf[j] == '{') {
          open++;
        }
        if (buf[j] == '}') {
          open--;
          if (open == 0) {
            break;
          }
        }
        if (open == 1 && buf[j] == ';') {
          argIdx++;
        } else {
          argLengths[argIdx]++;
        }
      }

      char **strArgs = malloc(sizeof(char *) * argc);

      for (int j = 0; j < argc; j++) {
        strArgs[j] = malloc(argLengths[j] + 1);
        memcpy(strArgs[j], buf + i, argLengths[j]);
        strArgs[j][argLengths[j]] = '\0';
        i += argLengths[j] + 1; // +1 for the semicolon
      }

      for (int j = 0; j < argc; j++) {
        args[j] = tokenize(strArgs[j], argLengths[j]);
        free(strArgs[j]);
      }

      free(argLengths);
      free(strArgs);

      bool returns = true;

      if (argc != 0) {
        if (args[argc - 1].length == 0) {
          returns = false;
          free(args[argc - 1].tokens);
          argc--;
        }
      }

      Token token = {0};
      token.type = TT_BLOCK;
      token.value = malloc(sizeof(BlockToken));
      ((BlockToken *)token.value)->stmts = args;
      ((BlockToken *)token.value)->stmtc = argc;
      append_token(&vec, token);
    } else {
      i++;
    }
  }
  return vec;
}

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

int main() {
  FILE *file;
  errno_t err = fopen_s(&file, "main.pv", "r");
  if (err)
    return err;
  struct stat stats;
  err = fstat(_fileno(file), &stats);
  if (err)
    return err;

  char *buf = malloc(stats.st_size + 1);

  int read = fread(buf, 1, stats.st_size, file);

  buf[read] = '\0';

  fclose(file);

  TokenVec tokens = tokenize(buf, read);

  free(buf);

  // for (int i = 0; i < tokens.length; i++) {
  //   print_token(tokens.tokens[i]);
  // }

  // return 0;

  Expr expr = {.type = EXPR_NULL, .value = NULL};
  char *error = parse(&expr, tokens);
  if (error) {
    printf("Error: %s\n", error);
    return 1;
  }

  print_expr(expr);
  printf("\n");

  free_expr(expr);

  report_leaks();

  return 0;
}