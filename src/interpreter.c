#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ast.h"
#include "env.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

value_t visit_literal(expr_t *expr)
{
	return expr->as.literal.value;
}

value_t visit_grouping(expr_t *expr, ht_t *env)
{
	return evaluate(expr->as.grouping.expression, env);
}

void runtime_error(const char *message, int line)
{
	fprintf(stderr, "%s\n[line %d]\n", message, line);
	errno = 70;
	exit(70);
}

value_t visit_binary(expr_t *expr, ht_t *env)
{
	token_type_t op_type = expr->as.binary.operator.type;
	value_t right = evaluate(expr->as.binary.right, env);
	value_t left = evaluate(expr->as.binary.left, env);

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
	if ((left.type == VAL_STRING && right.type == VAL_NUMBER) ||
			(left.type == VAL_NUMBER && right.type == VAL_STRING)) {
		runtime_error("Operands must be numbers.", expr->line);
	}

	runtime_error("Operands must be two numbers or two strings.", expr->line);
	return (value_t){.type = VAL_NIL};

}

int is_truthy(value_t value)
{
	switch (value.type) {
		case VAL_NIL:
			return 0;

		case VAL_BOOL:
			return value.as.boolean;

		case VAL_NUMBER:
			if (value.as.number == 0)
				return 0;
			return 1;

		case VAL_STRING:
			return 1;

		default:
			return 0;
	}
}

value_t visit_unary(expr_t *expr, ht_t *env)
{
	value_t operand = evaluate(expr->as.unary.right, env);

	if (expr->as.unary.operator.type == TOKEN_MINUS) {
		if (operand.type == VAL_NUMBER) {
			value_t result = {.type = VAL_NUMBER, .as.number = -operand.as.number};
			return result;
		} else {
			runtime_error("Operand must be a number.", expr->line);
		}
	} else if (expr->as.unary.operator.type == TOKEN_BANG) {
		value_t result = {.type = VAL_BOOL, .as.boolean = !is_truthy(operand)};
		return result;
	}

	return (value_t){.type = VAL_NIL};
}

value_t visit_variable(expr_t *expr, ht_t *env)
{
	value_t *val = ht_get(env, &expr->as.variable.name, 1);
	if (val) {
		return *val;
	} else {
		return (value_t) {.type = VAL_NIL};
	}
}

value_t visit_assign(expr_t *expr, ht_t *env)
{
	value_t value = evaluate(expr->as.assign.value, env);
	ht_assign(env, &expr->as.assign.name->as.variable.name, value);
    return value;
}

value_t visit_logical(expr_t *expr, ht_t *env)
{
	value_t left = evaluate(expr->as.logical.left, env);

    if (expr->as.logical.operator.type == TOKEN_OR) {
      if (is_truthy(left))
		  return left;
    } else {
      if (!is_truthy(left))
		  return left;
    }

    return evaluate(expr->as.logical.right, env);
}

value_t evaluate(expr_t *expr, ht_t *env)
{
	if (!expr) {
		value_t nil_value = {.type = VAL_NIL };
		return nil_value;
	}
	switch (expr->type) {
		case EXPR_LITERAL:
			return visit_literal(expr);
		case EXPR_BINARY:
			return visit_binary(expr, env);
		case EXPR_UNARY:
			return visit_unary(expr, env);
		case EXPR_GROUPING:
			return visit_grouping(expr, env);
		case EXPR_VARIABLE:
			return visit_variable(expr, env);
		case EXPR_ASSIGN:
			return visit_assign(expr, env);
		case EXPR_LOGICAL:
			return visit_logical(expr, env);
		default:
			exit(65);
			break;
	}
}

void print_value(value_t value)
{
	switch (value.type) {
		case VAL_BOOL:
			printf("%s\n", value.as.boolean == 1 ? "true" : "false");
			break;

		case VAL_NIL:
			printf("nil\n");
			break;

		case VAL_STRING:
			printf("%s\n", value.as.string);
			break;

		case VAL_NUMBER:
			if (value.as.number == (int) value.as.number) {
				printf("%d\n", (int) value.as.number);
			} else {
				printf("%g\n", value.as.number);
			}
			break;

		default:
			break;
	}
}

void evaluate_block(stmt_array_t *array, ht_t *cur_env, ht_t *scope_env)
{
	ht_t *previous = cur_env;
	cur_env = scope_env;
	evaluate_statements(array, cur_env);
	ht_free(scope_env);
	cur_env = previous;
}

void evaluate_statement(stmt_t *stmt, ht_t *env)
{
	switch (stmt->type) {
		case STMT_IF:
			if (is_truthy(evaluate(stmt->as._if.condition, env))) {
				evaluate_statement(stmt->as._if.then_branch, env);
			} else if (stmt->as._if.else_branch) {
				evaluate_statement(stmt->as._if.else_branch, env);
			}
			break;

		case STMT_PRINT:
			print_value(evaluate(stmt->as.print.expression, env));
			break;

		case STMT_EXPR:
			evaluate(stmt->as.expr.expression, env);
			break;

		case STMT_VAR: {
			value_t value = {.type = VAL_NIL};
			if (stmt->as.variable.initializer) {
				value = evaluate(stmt->as.variable.initializer, env);
			}
			ht_add(env, stmt->as.variable.name.value, value);
			break;
		}

		case STMT_BLOCK:
			evaluate_block(stmt->as.block.statements, env, ht_init(env));
			break;

		case STMT_WHILE:
			while (is_truthy(evaluate(stmt->as._while.condition, env))) {
				evaluate_statement(stmt->as._while.body, env);
			}
			break;

		default:
			break;
	}
}

void evaluate_statements(stmt_array_t *array, ht_t *env)
{
	for (int i = 0; i < array->length; i++) {
		evaluate_statement(array->statements[i], env);
	}
}
