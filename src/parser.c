#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

int current = 0;
token_t *tokens;

expr_t *expression(void);

void error(token_t *token, char *message)
{
	if (token->type == END_OF_FILE) {
		fprintf(stderr, "[line %d] at end: %s\n", token->line, message);
	} else {
		fprintf(stderr, "[line %d] at '%s': %s\n", token->line, token->value, message);
	}
	errno = 65;
}

void free_expr(expr_t *expr)
{
	if (!expr)
		return;
	switch (expr->type) {
		case BINARY:
			free(expr->as.binary.binary_op.value);
			free_expr(expr->as.binary.left);
			free_expr(expr->as.binary.right);
			free(expr);
			break;

		case GROUPING:
			free_expr(expr->as.grouping.expression);
			free(expr);
			break;

		case UNARY:
			free(expr->as.unary.unary_op.value);
			free_expr(expr->as.unary.right);
			free(expr);
			break;

		case LITERAL:
			if (expr->as.literal.value.type == VAL_STRING) {
				free(expr->as.literal.value.as.string);
			}
			free(expr);
			break;

		default:
			break;
	}
}

expr_t *parse(token_t *tks)
{
	tokens = tks;
	if (errno == 65) {
		return NULL;
	} else {
		return expression();
	}
}

token_t *peek(void)
{
	return &tokens[current];
}

int isAtEnd(void)
{
	return tokens[current].type == END_OF_FILE;
}

token_t *previous(void)
{
	return &tokens[current - 1];
}

void advance(void)
{
	if (!isAtEnd())
		current++;
}


int check(token_type_t type)
{
	if (tokens[current].type == type) {
		advance();
		return 1;
	} else {
		return 0;
	}
}

void consume(token_type_t type, char *message) {
	if (!check(type)) {
		error(peek(), message);
	}
}

expr_t *primary(void)
{
	if (check(FALSE) || check(TRUE) || check(NIL) || check(NUMBER) || check(STRING)) {
		return create_literal_expr(previous());
	}
	if (check(LEFT_PAREN)) {
		expr_t *expr = expression();
		consume(RIGHT_PAREN, "Expect ')' after expression.");
		return create_grouping_expr(expr);
	}
	error(peek(), "Expect expression.");
	return NULL;
}

expr_t *unary(void)
{
	if (check(BANG) || check(MINUS)) {
		token_t *operator = previous();
		expr_t *right = unary();
		return create_unary_expr(operator, right);
	}

	return primary();
}

expr_t *factor(void)
{
	expr_t *expr = unary();

	while (check(SLASH) || check(STAR)) {
		token_t *operator = previous();
		expr_t *right = unary();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *term(void)
{
	expr_t *expr = factor();

	while (check(MINUS) || check(PLUS)) {
		token_t *operator = previous();
		expr_t *right = factor();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *comparison(void)
{
	expr_t *expr = term();

	while (check(GREATER) || check(GREATER_EQUAL) || check(LESS) || check(LESS_EQUAL)) {
		token_t *operator = previous();
		expr_t *right = term();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *equality(void)
{
	expr_t *expr = comparison();

	while (check(BANG_EQUAL) || check(EQUAL_EQUAL)) {
		token_t *operator = previous();
		expr_t *right = comparison();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *expression(void)
{
	return equality();
}

void synchronize(void)
{
	advance();

	while (!isAtEnd()) {
		if (previous()->type == SEMICOLON) return;

		switch (peek()->type) {
			case CLASS:
			case FUN:
			case VAR:
			case FOR:
			case IF:
			case WHILE:
			case PRINT:
			case RETURN:
				return;
			default:
				return;
		}

		advance();
	}
}
