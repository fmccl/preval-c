#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memtracker.h"
#include "operator.h"
#include "tokeniser.h"

void append_token(TokenVec *vec, Token token) {
  if (vec->capacity == vec->length) {
    vec->capacity += 1;
    vec->capacity *= 2;
    vec->tokens = realloc(vec->tokens, vec->capacity * sizeof(Token));
  }
  vec->tokens[vec->length] = token;
  vec->length++;
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
    strcpy((char *)newToken.value, (char *)token.value);
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

TokenVec tokenize(char *buf, size_t len) {

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
        *(float *)token.value = (float)atof(numStr);
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