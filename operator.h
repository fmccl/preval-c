#ifndef OPERATOR_H
#define OPERATOR_H
typedef enum {
  OP_ADD = '+',
  OP_SUB = '-',
  OP_MUL = '*',
  OP_DIV = '/',
  OP_ASSIGN = '=',
  OP_ARROW,
} Operator;

int precidence(Operator op);
#endif