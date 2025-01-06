#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"

expr_t *create_binary_expr(token_t *binary_op, expr_t *left, expr_t *right)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = BINARY;
	expr->line = binary_op->line;
	expr->as.binary.left = left;
	expr->as.binary.right = right;
	expr->as.binary.binary_op.type = binary_op->type;
	char *bin_op_val = strdup(binary_op->value);
	expr->as.binary.binary_op.value = bin_op_val;
	expr->as.binary.binary_op.line = binary_op->line;
	return expr;
}

expr_t *create_unary_expr(token_t *unary_op, expr_t *right)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = UNARY;
	expr->line = unary_op->line;
	expr->as.unary.unary_op.type = unary_op->type;
	char *u_op_val = strdup(unary_op->value);
	expr->as.unary.unary_op.value = u_op_val;
	expr->as.unary.unary_op.line = unary_op->line;
	expr->as.unary.right = right;
	return expr;
}

expr_t *create_literal_expr(token_t *token)
{
	expr_t *expr = malloc(sizeof(expr_t));
	expr->type = LITERAL;
	expr->line = token->line;
	switch (token->type) {
		case NUMBER:
			expr->as.literal.value.type = VAL_NUMBER;
			double num;
			sscanf(token->value, "%lf", &num);
			expr->as.literal.value.as.number = num;
			break;

		case NIL:
			expr->as.literal.value.type = VAL_NIL;
			expr->as.literal.value.as.number = 0;
			break;

		case TRUE:
		case FALSE:
			expr->as.literal.value.type = VAL_BOOL;
			expr->as.literal.value.as.boolean = token->type == TRUE;
			break;

		case STRING:
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
	expr->type = GROUPING;
	expr->line = expression->line;
	expr->as.grouping.expression = expression;
	return expr;
}

void print_ast(expr_t *expr)
{
	if (!expr)
		return;
	if (expr->type == LITERAL) {
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
	} else if (expr->type == BINARY) {
		printf("(%s ", expr->as.binary.binary_op.value);
		print_ast(expr->as.binary.left);
		printf(" ");
		print_ast(expr->as.binary.right);
		printf(")");
	} else if (expr->type == UNARY) {
		printf("(%s ", expr->as.unary.unary_op.value);
		print_ast(expr->as.unary.right);
		printf(")");
	} else if (expr->type == GROUPING) {
		printf("(group ");
		print_ast(expr->as.grouping.expression);
		printf(")");
	}
}
