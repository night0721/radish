#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

int current = 0;
token_t *tokens;
expr_t *expression(void);
void synchronize(void);

/*
 * Syntax error
 */
void error(token_t *token, char *message)
{
	if (token->type == TOKEN_EOF) {
		fprintf(stderr, "[line %d] at end: %s\n", token->line, message);
	} else {
		fprintf(stderr, "[line %d] at '%s': %s\n", token->line, token->value, message);
	}
	errno = 65;
	synchronize();
}

void free_expr(expr_t *expr)
{
	if (!expr)
		return;
	switch (expr->type) {
		case EXPR_BINARY:
			free(expr->as.binary.operator.value);
			free_expr(expr->as.binary.left);
			free_expr(expr->as.binary.right);
			free(expr);
			break;

		case EXPR_GROUPING:
			free_expr(expr->as.grouping.expression);
			free(expr);
			break;

		case EXPR_UNARY:
			free(expr->as.unary.operator.value);
			free_expr(expr->as.unary.right);
			free(expr);
			break;

		case EXPR_LITERAL:
			if (expr->as.literal.value.type == VAL_STRING) {
				free(expr->as.literal.value.as.string);
			}
			free(expr);
			break;
	
		case EXPR_VARIABLE:
			free(expr->as.variable.name.value);
			free(expr);
			break;

		case EXPR_ASSIGN:
			free(expr->as.assign.name.value);
			free_expr(expr->as.assign.value);
			break;

		default:
			break;
	}
}

token_t *peek(void)
{
	return &tokens[current];
}

int end(void)
{
	return tokens[current].type == TOKEN_EOF;
}

token_t *previous(void)
{
	return &tokens[current - 1];
}

void advance(void)
{
	if (!end())
		current++;
}

int check(token_type_t type)
{
	return tokens[current].type == type;
}

token_t *consume(token_type_t type, char *message)
{
	if (!check(type)) {
		error(peek(), message);
	} else {
		token_t *tok = peek();
		advance();
		return tok;
	}
	return NULL;
}

expr_t *primary(void)
{
	if (check(TOKEN_FALSE) || check(TOKEN_TRUE) || check(TOKEN_NIL) ||
			check(TOKEN_NUMBER) || check(TOKEN_STRING)) {
		token_t *tok = peek();
		advance();
		return create_literal_expr(tok);
	}

	if (check(TOKEN_IDENTIFIER)) {
		token_t *tok = peek();
		advance();
		return create_variable_expr(tok);
	}

	if (check(TOKEN_LEFT_PAREN)) {
		advance();
		expr_t *expr = expression();
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
		return create_grouping_expr(expr);
	}
	error(peek(), "Expect expression.");
	return NULL;
}

expr_t *unary(void)
{
	if (check(TOKEN_BANG) || check(TOKEN_MINUS)) {
		token_t *operator = peek();
		advance();
		expr_t *right = unary();
		return create_unary_expr(operator, right);
	}

	return primary();
}

expr_t *factor(void)
{
	expr_t *expr = unary();

	while (check(TOKEN_SLASH) || check(TOKEN_STAR)) {
		token_t *operator = peek();
		advance();
		expr_t *right = unary();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *term(void)
{
	expr_t *expr = factor();

	while (check(TOKEN_MINUS) || check(TOKEN_PLUS)) {
		token_t *operator = peek();
		advance();
		expr_t *right = factor();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *comparison(void)
{
	expr_t *expr = term();

	while (check(TOKEN_GREATER) || check(TOKEN_GREATER_EQUAL) || check(TOKEN_LESS)
			|| check(TOKEN_LESS_EQUAL)) {
		token_t *operator = peek();
		advance();
		expr_t *right = term();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *equality(void)
{
	expr_t *expr = comparison();

	while (check(TOKEN_BANG_EQUAL) || check(TOKEN_EQUAL_EQUAL)) {
		token_t *operator = peek();
		advance();
		expr_t *right = comparison();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *assignment(void)
{
	expr_t *expr = equality();

	if (check(TOKEN_EQUAL)) {
		token_t *equals = peek();
		advance();
		expr_t *value = assignment();

		if (expr->type == EXPR_VARIABLE) {
			token_t name = expr->as.variable.name;
			return create_assign_expr(&name, value);
		}
		error(equals, "Invalid assignment target.");
	}

    return expr;
}

expr_t *expression(void)
{
	return assignment();
}

stmt_t print_stmt(void)
{
	expr_t *value = expression();
	consume(TOKEN_SEMICOLON, "Expect ; after value.");
	return (stmt_t) {
		.type = STMT_PRINT,
		.as.print.expression = value,
	};
}

stmt_t expression_stmt(void)
{
	expr_t *expr = expression();
	consume(TOKEN_SEMICOLON, "Expect ; after expression.");
	return (stmt_t) {
		.type = STMT_EXPR,
		.as.expr.expression = expr,
	};
}

stmt_t statement(void)
{
	if (check(TOKEN_PRINT)) {
		advance();
		return print_stmt();
	}
	return expression_stmt();
}

stmt_t var_declaration(void)
{
	token_t *name = consume(TOKEN_IDENTIFIER, "Expect variable name.");

	expr_t *initializer = NULL;
	if (check(TOKEN_EQUAL)) {
		advance();
		initializer = expression();
	}

	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
	return (stmt_t) {
		.type = STMT_VAR,
		.as.variable.name.type = name->type,
		.as.variable.name.value = name->value,
		.as.variable.name.line = name->line,
		.as.variable.initializer = initializer,
	};
}

stmt_t declaration(void)
{
	if (check(TOKEN_VAR)) {
		advance();
		return var_declaration();
	}

	return statement();
}

void stmt_add(stmt_array_t *array, stmt_t stmt)
{
	if (array->length == array->capacity) {
		array->capacity *= 2;
		array->statements = realloc(array->statements, array->capacity * sizeof(stmt_t));
	}
	array->statements[array->length++] = stmt;
}

void free_statements(stmt_array_t *array)
{
	for (int i = 0; i < array->length; i++) {
		if (array->statements[i].type == STMT_PRINT) {
			free_expr(array->statements[i].as.print.expression);
		}
		if (array->statements[i].type == STMT_EXPR) {
			free_expr(array->statements[i].as.expr.expression);
		}
		if (array->statements[i].type == STMT_VAR) {
			free_expr(array->statements[i].as.variable.initializer);
		}
	}
	free(array->statements);
	free(array);
}

stmt_array_t *parse(token_t *tks)
{
	tokens = tks;
	if (errno == 65) {
		return NULL;
	} else {
		stmt_array_t *statements = malloc(sizeof(stmt_array_t));
		statements->statements = malloc(DEFAULT_STMTS_SIZE * sizeof(stmt_t));
		statements->length = 0;
		statements->capacity = DEFAULT_STMTS_SIZE;
		while (!end()) {
			stmt_add(statements, declaration());
		}
		return statements;
	}
}

expr_t *parse_expr(token_t *tks)
{
	tokens = tks;
	if (errno == 65) {
		return NULL;
	} else {
		return expression();
	}
}

void synchronize(void)
{
	advance();

	while (!end()) {
		if (previous()->type == TOKEN_SEMICOLON) return;

		switch (peek()->type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;
			default:
				return;
		}

		advance();
	}
}
