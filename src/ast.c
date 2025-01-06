#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"

expr_t *create_binary_expr(token_t *operator, expr_t *left, expr_t *right)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = EXPR_BINARY;
	expr->line = operator->line;
	expr->as.binary.left = left;
	expr->as.binary.right = right;
	expr->as.binary.operator.type = operator->type;
	char *bin_op_val = strdup(operator->value);
	expr->as.binary.operator.value = bin_op_val;
	expr->as.binary.operator.line = operator->line;
	return expr;
}

expr_t *create_unary_expr(token_t *operator, expr_t *right)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = EXPR_UNARY;
	expr->line = operator->line;
	expr->as.unary.operator.type = operator->type;
	char *u_op_val = strdup(operator->value);
	expr->as.unary.operator.value = u_op_val;
	expr->as.unary.operator.line = operator->line;
	expr->as.unary.right = right;
	return expr;
}

expr_t *create_literal_expr(token_t *token)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = EXPR_LITERAL;
	expr->line = token->line;
	switch (token->type) {
		case TOKEN_NUMBER:
			expr->as.literal.value.type = VAL_NUMBER;
			double num;
			sscanf(token->value, "%lf", &num);
			expr->as.literal.value.as.number = num;
			break;

		case TOKEN_NIL:
			expr->as.literal.value.type = VAL_NIL;
			expr->as.literal.value.as.number = 0;
			break;

		case TOKEN_TRUE:
		case TOKEN_FALSE:
			expr->as.literal.value.type = VAL_BOOL;
			expr->as.literal.value.as.boolean = token->type == TOKEN_TRUE;
			break;

		case TOKEN_STRING:
			expr->as.literal.value.type = VAL_STRING;
			char *tkvalue = strdup(token->value);
			expr->as.literal.value.as.string = tkvalue;
			break;

		default:
			break;
	}
	return expr;
}

expr_t *create_grouping_expr(expr_t *expression)
{
	if (!expression) {
		return NULL;
	}
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = EXPR_GROUPING;
	expr->line = expression->line;
	expr->as.grouping.expression = expression;
	return expr;
}

expr_t *create_variable_expr(token_t *name)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = EXPR_VARIABLE;
	expr->line = name->line;
	expr->as.variable.name.type = name->type;
	char *name_val = strdup(name->value);
	expr->as.variable.name.value = name_val;
	expr->as.variable.name.line = name->line;

	return expr;
}

void print_ast(expr_t *expr)
{
	if (!expr)
		return;
	if (expr->type == EXPR_LITERAL) {
		switch (expr->as.literal.value.type) {
			case VAL_BOOL:
				printf("%s", expr->as.literal.value.as.boolean ? "true" : "false");
				break;

			case VAL_NIL:
				printf("nil");
				break;

			case VAL_NUMBER:;
				double value = expr->as.literal.value.as.number;
				if (value == (int) value) {
					printf("%.1f", value);
				} else {
					printf("%g", value);
				}
				break;

			case VAL_STRING:
				printf("%s", expr->as.literal.value.as.string);
				break;
		}
	} else if (expr->type == EXPR_BINARY) {
		printf("(%s ", expr->as.binary.operator.value);
		print_ast(expr->as.binary.left);
		printf(" ");
		print_ast(expr->as.binary.right);
		printf(")");
	} else if (expr->type == EXPR_UNARY) {
		printf("(%s ", expr->as.unary.operator.value);
		print_ast(expr->as.unary.right);
		printf(")");
	} else if (expr->type == EXPR_GROUPING) {
		printf("(group ");
		print_ast(expr->as.grouping.expression);
		printf(")");
	}
}
