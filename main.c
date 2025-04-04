#include <stdlib.h>

#include "compiler.h"
#include "memtracker.h"
#include "parser.h"
#include "sb.h"
#include "tokeniser.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main() {
  FILE *file = fopen("main.pv", "r");
  if (!file) {
    fprintf(stderr, "Failed to open file\n");
    return 1;
  }
  struct stat stats;
  int err = fstat(fileno(file), &stats);
  if (err)
    return err;

  char *buf = malloc(stats.st_size + 1);

  size_t read = fread(buf, 1, stats.st_size, file);

  buf[read] = '\0';

  fclose(file);

  TokenVec tokens = tokenize(buf, read);

  free(buf);

  // for (int i = 0; i < tokens.length; i++) {
  //   print_token(tokens.tokens[i]);
  // }

  // return 0;

  Expr expr = {.type = EXPR_NULL, .value = NULL};
  char *error = parse(&expr, tokens, true);
  if (error) {
    printf("Error: %s\n", error);
    return 1;
  }

  print_expr(expr);
  printf("\n");

  if (expr.type == EXPR_FUNC) {
    StringBuilder impl = {0};
    StringBuilder decl = {0};
    compile_function(&decl, &impl, *expr.value.func, "main");
    StringBuilder outBuilder = {0};
    char *declStr = sb_to_string(&decl, true);
    char *implStr = sb_to_string(&impl, true);
    sb_write(&outBuilder, declStr);
    sb_write(&outBuilder, implStr);
    free(declStr);
    free(implStr);
    FILE *outFile = fopen("out.ll", "w");
    if (file == NULL) {
      printf("Failed to open out.ll");
      return 1;
    }
    char *out = sb_to_string(&outBuilder, true);
    fputs(out, file);
    free(out);
  }

  free_expr(expr);

  report_leaks();

  return 0;
}