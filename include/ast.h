#ifndef AST_H
#define AST_H

#include "lexer.h"

#define DEFAULT_STMTS_SIZE 512
#define DEFAULT_ARGS_SIZE 255

/*
expression → equality ;
equality   → comparison ( ( "!=" | "==" ) comparison )* ;
comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term       → factor ( ( "-" | "+" ) factor )* ;
factor     → unary ( ( "/" | "*" ) unary )* ;
unary      → ( "!" | "-" ) unary | primary ;
primary    → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
statement      → exprStmt | ifStmt | printStmt | block ;
ifStmt         → "if" "(" expression ")" statement ( "else" statement )? ;
block          → "{" declaration* "}" ;
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
	VAL_FN,
} value_type_t;

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

typedef enum {
	FN_NATIVE,
	FN_CUSTOM,
} fn_type_t;

typedef struct expr_t expr_t;
typedef struct value_t value_t;
typedef struct stmt_t stmt_t;
typedef struct fn_t fn_t;

typedef struct {
	expr_t **arguments;
	int length;
	int capacity;
} arg_array_t;

typedef struct {
	value_t **arguments;
	int length;
	int capacity;
} val_array_t;

typedef struct {
	stmt_t **statements;
	int length;
	int capacity;
} stmt_array_t;

struct value_t {
	value_type_t type;
	union {
		int boolean;
		double number;
		char *string;
		fn_t *function;
	} as;
};

typedef struct ht_t ht_t;

struct fn_t {
	fn_type_t type;
	int arity;
	stmt_t *stmt;
	value_t *(*call)(stmt_t *stmt, val_array_t *arguments, ht_t *env);
};

struct expr_t {
	expr_type_t type;
	int line;
	union {
		struct {
			struct expr_t *name;
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
			arg_array_t *args;
		} call;
		struct {
			struct expr_t *object;
			token_t name;
		} get;
		struct {
			struct expr_t *expression;
		} grouping;
		struct {
			value_t *value;
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
};

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
			struct stmt_t *body;
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

expr_t *create_binary_expr(token_t *operator, expr_t *left, expr_t *right);
expr_t *create_unary_expr(token_t *operator, expr_t *right);
expr_t *create_literal_expr(token_t *token);
expr_t *create_grouping_expr(expr_t *expression);
expr_t *create_variable_expr(token_t *name);
expr_t *create_assign_expr(expr_t *name, expr_t *value);
expr_t *create_logical_expr(token_t *operator, expr_t *left, expr_t *right);
expr_t *create_call_expr(expr_t *callee, token_t *paren, arg_array_t *args);
void print_ast(expr_t *expr);

#endif
