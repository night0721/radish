#ifndef AST_H
#define AST_H

#include "lexer.h"

/*
expression     → literal
               | unary
               | binary
               | grouping ;
literal        → NUMBER | STRING | "true" | "false" | "nil" ;
grouping       → "(" expression ")" ;
unary          → ( "-" | "!" ) expression ;
binary         → expression operator expression ;
operator       → "==" | "!=" | "<" | "<=" | ">" | ">="
               | "+"  | "-"  | "*" | "/" ;
*/

typedef enum {
	BINARY,
	UNARY,
    LITERAL,
    GROUPING,
} expr_type;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING,
} value_type_t;

typedef struct {
    value_type_t type;
    union {
        int boolean;
        double number;
        char *string;
    } as;
} value_t;

typedef struct expr_t {
    expr_type type;
    int line;
    union {
        struct {
            token_t binary_op;
            struct expr_t *left;
            struct expr_t *right;
        } binary;
        struct {
            token_t unary_op;
            struct expr_t *right;
        } unary;
        struct {
            value_t value;
        } literal;
		struct {
			struct expr_t *expression;
		} grouping;
    } as;
} expr_t;

expr_t *create_binary_expr(token_t *binary_op, expr_t *left, expr_t *right);
expr_t *create_unary_expr(token_t *unary_op, expr_t *right);
expr_t *create_literal_expr(token_t *token);
expr_t *create_grouping_expr(expr_t *expression);
void print_ast(expr_t *expr);

#endif
