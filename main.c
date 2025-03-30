#include <stdlib.h>

#include "memtracker.h"
#include "parser.h"
#include "tokeniser.h"
#include <corecrt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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
  char *error = parse(&expr, tokens, true);
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