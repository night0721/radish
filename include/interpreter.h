#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "env.h"
#include "stmt.h"

void runtime_error(const char *message, int line);
value_t evaluate(expr_t *expr, ht_t *env);
void print_value(value_t value);
void evaluate_statements(stmt_array_t *array, ht_t *env);

#endif
