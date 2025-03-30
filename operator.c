#include "operator.h"

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