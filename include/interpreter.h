#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "stmt.h"

void runtime_error(const char *message, int line);
value_t evaluate(expr_t *expr);
void print_value(value_t *value);
void print_statements(stmt_array_t *array);

#endif
