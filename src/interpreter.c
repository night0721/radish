#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ast.h"
#include "interpreter.h"
#include "lexer.h"

value_t visit_literal(expr_t *expr);
value_t visit_grouping(expr_t *expr);
value_t visit_binary(expr_t *expr);
value_t visit_unary(expr_t *expr);

value_t visit_literal(expr_t *expr)
{
	return expr->as.literal.value;
}

value_t visit_grouping(expr_t *expr)
{
	return evaluate(expr->as.grouping.expression);
}

void runtime_error(const char *message, int line)
{
	fprintf(stderr, "%s\n[line %d]\n", message, line);
	errno = 70;
	exit(70);
}

value_t visit_binary(expr_t *expr)
{
	token_type_t op_type = expr->as.binary.operator.type;
	value_t right = evaluate(expr->as.binary.right);
	value_t left = evaluate(expr->as.binary.left);

	// Arithmetic
	if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
		value_t result = {.type = VAL_NUMBER};
		switch (op_type) {
			case TOKEN_PLUS:
				result.as.number = left.as.number + right.as.number;
				return result;

			case TOKEN_MINUS:
				result.as.number = left.as.number - right.as.number;
				return result;

			case TOKEN_STAR:
				result.as.number = left.as.number * right.as.number;
				return result;

			case TOKEN_SLASH:
				if (right.as.number == 0) {
					runtime_error("Division by zero.", expr->line);
				}
				result.as.number = left.as.number / right.as.number;
				return result;
			default:
				break;
		}
	}

	// Comparison
	if (op_type == TOKEN_EQUAL_EQUAL || op_type == TOKEN_BANG_EQUAL) {
		int is_equal;
		if (left.type != right.type) {
			is_equal = 0;
		} else {
			switch (left.type) {
				case VAL_NUMBER:
					is_equal = left.as.number == right.as.number;
					break;

				case VAL_BOOL:
					is_equal = left.as.boolean == right.as.boolean;
					break;

				case VAL_STRING:
					is_equal = strcmp(left.as.string, right.as.string) == 0;
					break;

				case VAL_NIL:
					is_equal = 1; // nil == nil
					break;

				default:
					is_equal = 0;
					break;
			}
		}
		value_t result = {.type = VAL_BOOL};
		result.as.boolean = op_type == TOKEN_EQUAL_EQUAL ? is_equal : !is_equal;
		return result;
	}

	// Number Comparison
	if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
		value_t result = {.type = VAL_BOOL };
		switch (op_type) {
			case TOKEN_GREATER:
				result.as.boolean= left.as.number > right.as.number;
				return result;

			case TOKEN_GREATER_EQUAL:
				result.as.boolean = left.as.number >= right.as.number;
				return result;

			case TOKEN_LESS:
				result.as.boolean = left.as.number < right.as.number;
				return result;

			case TOKEN_LESS_EQUAL:
				result.as.boolean = left.as.number <= right.as.number;
				return result;

			default: break;
		}
	}

	// String concatenation
	if (left.type == VAL_STRING && right.type == VAL_STRING) {
		if (op_type == TOKEN_PLUS) {
			value_t result = {.type = VAL_STRING};
			size_t left_len = strlen(left.as.string);
			size_t right_len = strlen(right.as.string);
			result.as.string = malloc(left_len + right_len + 1);
			strcpy(result.as.string, left.as.string);
			strcat(result.as.string, right.as.string);
			return result;
		}
	}

	// String/number comparisons
	if (left.type == VAL_STRING && right.type == VAL_NUMBER ||
			left.type == VAL_NUMBER && right.type == VAL_STRING ) {
		runtime_error("Operands must be numbers.", expr->line);

	}

	runtime_error("Operands must be two numbers or two strings.", expr->line);
	return (value_t){.type = VAL_NIL};

}

int is_truthy(value_t *value)
{
	switch (value->type) {
		case VAL_NIL:
			return 0;

		case VAL_BOOL:
			return value->as.boolean;

		case VAL_NUMBER:
			if (value->as.number == 0)
				return 0;
			return 1;

		case VAL_STRING:
			return 1;

		default:
			return 0;
	}
}

value_t visit_unary(expr_t *expr)
{
	value_t operand = evaluate(expr->as.unary.right);

	if (expr->as.unary.operator.type == TOKEN_MINUS) {
		if (operand.type == VAL_NUMBER) {
			value_t result = {.type = VAL_NUMBER, .as.number = -operand.as.number};
			return result;
		} else {
			runtime_error("Operand must be a number.", expr->line);
		}
	} else if (expr->as.unary.operator.type == TOKEN_BANG) {
		value_t result = {.type = VAL_BOOL, .as.boolean = !is_truthy(&operand)};
		return result;
	}

	return (value_t){.type = VAL_NIL};
}

value_t evaluate(expr_t *expr)
{
	if (!expr) {
		value_t nil_value = {.type = VAL_NIL };
		return nil_value;
	}
	switch (expr->type) {
		case EXPR_LITERAL:
			return visit_literal(expr);
		case EXPR_BINARY:
			return visit_binary(expr);
		case EXPR_UNARY:
			return visit_unary(expr);
		case EXPR_GROUPING:
			return visit_grouping(expr);
		default:
			exit(65);
			break;
	}
}

void print_value(value_t *value)
{
	switch (value->type) {
		case VAL_BOOL:
			printf("%s\n", value->as.boolean == 1 ? "true" : "false");
			break;

		case VAL_NIL:
			printf("nil\n");
			break;

		case VAL_STRING:
			printf("%s\n", value->as.string);
			break;

		case VAL_NUMBER:
			if (value->as.number == (int) value->as.number) {
				printf("%d\n", (int) value->as.number);
			} else {
				printf("%g\n", value->as.number);
			}
			break;

		default:
			break;
	}
}

void print_statement(stmt_t stmt)
{
	if (stmt.type == STMT_PRINT) {
		value_t obj = evaluate(stmt.as.print.expression);
		print_value(&obj);
	} else if (stmt.type == STMT_EXPR) {
		evaluate(stmt.as.expr.expression);
	}
}
void print_statements(stmt_array_t *array)
{
	for (int i = 0; i < array->length; i++) {
		print_statement(array->statements[i]);
	}
}
