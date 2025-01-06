#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

value_t evaluate(expr_t *expr);
void print_value(value_t *value);

#endif
