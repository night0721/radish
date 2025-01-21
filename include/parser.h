#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

stmt_array_t *parse(token_t *tks);
expr_t *parse_expr(token_t *tks);
void free_expr(expr_t *expr);
void free_statements(stmt_array_t *array);

#endif
