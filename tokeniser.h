#ifndef TOKENISER_H
#define TOKENISER_H

#include "operator.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct Token Token;
typedef struct TokenVec TokenVec;
typedef struct ParensToken ParensToken;
typedef struct BlockToken BlockToken;

struct TokenVec {
  int capacity;
  int length;
  Token *tokens;
};

struct ParensToken {
  TokenVec *args;
  int argc;
};

struct BlockToken {
  TokenVec *stmts;
  int stmtc;
  bool returns;
};

struct Token {
  enum {
    TT_INT,
    TT_FLOAT,
    TT_OP,
    TT_NAME,
    TT_PARENS,
    TT_BLOCK,
    TT_COLON,
  } type;
  union {
    int _int;
    float _float;
    Operator op;
    char *name;
    ParensToken *parens;
    BlockToken *block;
  } value;
};

void append_token(TokenVec *vec, Token token);
Token copy_token(Token token);
void print_token(Token token);
void free_token(Token token);
TokenVec tokenize(char *buf, size_t len);
#endif