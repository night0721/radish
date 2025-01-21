#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "env.h"

void free_val(value_t *value);
void runtime_error(const char *message, int line);
value_t *evaluate(expr_t *expr, ht_t *env);
void print_value(value_t *value);
void interpret(stmt_array_t *array);

#endif
