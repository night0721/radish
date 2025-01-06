#ifndef PARSER_H
#define PARSER_H

/*
expression → equality ;
equality   → comparison ( ( "!=" | "==" ) comparison )* ;
comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term       → factor ( ( "-" | "+" ) factor )* ;
factor     → unary ( ( "/" | "*" ) unary )* ;
unary      → ( "!" | "-" ) unary | primary ;
primary    → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
*/

#include "ast.h"
#include "lexer.h"
#include "stmt.h"

stmt_array_t *parse(token_t *tks);
expr_t *parse_expr(token_t *tks);
void free_expr(expr_t *expr);
void free_statements(stmt_array_t *array);

#endif
