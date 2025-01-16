#ifndef AST_H
#define AST_H

#include "lexer.h"

/*
   expression → literal | unary | binary | grouping ;
   literal   → NUMBER | STRING | "true" | "false" | "nil" ;
   grouping  → "(" expression ")" ;
   unary     → ( "-" | "!" ) expression ;
   binary    → expression operator expression ;
   operator  → "==" | "!=" | "<" | "<=" | ">" | ">=" | "+" | "-" | "*" | "/" ;
*/

typedef enum {
	EXPR_ASSIGN,
	EXPR_BINARY,
	EXPR_CALL,
	EXPR_GET,
	EXPR_GROUPING,
	EXPR_LITERAL,
	EXPR_LOGICAL,
	EXPR_SET,
	EXPR_SUPER,
	EXPR_THIS,
	EXPR_UNARY,
	EXPR_VARIABLE,
} expr_type_t;

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
	expr_type_t type;
	int line;
	union {
		struct {
			token_t name;
			struct expr_t *value;
		} assign;
		struct {
			token_t operator;
			struct expr_t *left;
			struct expr_t *right;
		} binary;
		struct {
			struct expr_t *callee;
			token_t paren;
			struct expr_t **arguments;
		} call;
		struct {
			struct expr_t *object;
			token_t name;
		} get;
		struct {
			struct expr_t *expression;
		} grouping;
		struct {
			value_t value;
		} literal;
		struct {
			token_t operator;
			struct expr_t *left;
			struct expr_t *right;
		} logical;
		struct {
			struct expr_t *object;
			token_t name;
			struct expr_t *value;
		} set;
		struct {
			token_t keyword;
			token_t method;
		} super;
		struct {
			token_t keyword;
		} this;
		struct {
			token_t operator;
			struct expr_t *right;
		} unary;
		struct {
			token_t name;
		} variable;
	} as;
} expr_t;

expr_t *create_binary_expr(token_t *operator, expr_t *left, expr_t *right);
expr_t *create_unary_expr(token_t *operator, expr_t *right);
expr_t *create_literal_expr(token_t *token);
expr_t *create_grouping_expr(expr_t *expression);
expr_t *create_variable_expr(token_t *name);
expr_t *create_assign_expr(token_t *name, expr_t *value);
void print_ast(expr_t *expr);

#endif
