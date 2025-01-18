#ifndef STMT_H
#define STMT_H

#include "ast.h"
#include "lexer.h"

#define DEFAULT_STMTS_SIZE 512

/*
statement      → exprStmt | ifStmt | printStmt | block ;
ifStmt         → "if" "(" expression ")" statement
               ( "else" statement )? ;
 block          → "{" declaration* "}" ;
*/

typedef enum {
	STMT_BLOCK,
	STMT_CLASS,
	STMT_EXPR,
	STMT_FUN,
	STMT_IF,
	STMT_PRINT,
	STMT_VAR,
	STMT_WHILE,
} stmt_type_t;

typedef struct stmt_t stmt_t;

typedef struct {
	stmt_t **statements;
	int length;
	int capacity;
} stmt_array_t;

struct stmt_t {
	stmt_type_t type;
	union {
		struct {
			stmt_array_t *statements;
		} block;
		struct {
			token_t name;
			token_t superclass;
			struct stmt_t **methods;
		} class;
		struct {
			expr_t *expression;
		} expr;
		struct {
			token_t name;
			array_t *params;
			struct stmt_t **body;
		} function;
		struct {
			expr_t *condition;
			struct stmt_t *then_branch;
			struct stmt_t *else_branch;
		} _if;
		struct {
			expr_t *expression;
		} print;
		struct {
			token_t keyword;
			expr_t *value;
		} _return;
		struct {
			token_t name;
			expr_t *initializer;
		} variable;
		struct {
			expr_t *condition;
			struct stmt_t *body;
		} _while;
	} as;
};

#endif 
