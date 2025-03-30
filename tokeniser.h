#ifndef TOKENISER_H
#define TOKENISER_H
#include <stdbool.h>
#include <stddef.h>

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

void append_token(TokenVec *vec, Token token);

Token copy_token(Token token);

void print_token(Token token);

void free_token(Token token);

TokenVec tokenize(char *buf, size_t len);
#endif