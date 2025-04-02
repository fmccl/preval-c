#ifndef COMPILER_H
#define COMPILER_H
#include "parser.h"
#include "sb.h"

void compile_function(StringBuilder *decl, StringBuilder *impl, FuncExpr func,
                      char *name);
#endif