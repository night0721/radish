#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ast.h"
#include "env.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

typedef struct {
    int has_returned;
    value_t *value;
} return_state_t;

void evaluate_statement(stmt_t *stmt, ht_t *env, return_state_t *state);

void free_val(value_t *value)
{
	if (!value)
		return;
	if (value->type == VAL_STRING) {
		if (value->as.string) {
			free(value->as.string);
		}
	} else if (value->type == VAL_FN) {
		if (value->as.function) {
			free(value->as.function);
		}
	}
	free(value);
}

value_t *visit_literal(expr_t *expr)
{
	value_t *val = malloc(sizeof(value_t));
	memcpy(val, expr->as.literal.value, sizeof(value_t));
	if (val->type == VAL_STRING) {
		val->as.string = strdup(expr->as.literal.value->as.string);
	}
	return val;
}

value_t *visit_grouping(expr_t *expr, ht_t *env)
{
	return evaluate(expr->as.grouping.expression, env);
}

void runtime_error(const char *message, int line)
{
	fprintf(stderr, "%s\n[line %d]\n", message, line);
	errno = 70;
	exit(70);
}

value_t *visit_binary(expr_t *expr, ht_t *env)
{
	token_type_t op_type = expr->as.binary.operator.type;
	value_t *right = evaluate(expr->as.binary.right, env);
	value_t *left = evaluate(expr->as.binary.left, env);

	// Arithmetic
	if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) {
		value_t *result = malloc(sizeof(value_t));
		result->type = VAL_NUMBER;
		switch (op_type) {
			case TOKEN_PLUS:
				result->as.number = left->as.number + right->as.number;
				free_val(left);
				free_val(right);
				return result;

			case TOKEN_MINUS:
				result->as.number = left->as.number - right->as.number;
				free_val(left);
				free_val(right);
				return result;

			case TOKEN_STAR:
				result->as.number = left->as.number * right->as.number;
				free_val(left);
				free_val(right);
				return result;

			case TOKEN_SLASH:
				if (right->as.number == 0) {
					runtime_error("Division by zero.", expr->line);
				}
				result->as.number = left->as.number / right->as.number;
				free_val(left);
				free_val(right);
				return result;
			default:
				break;
		}
		free_val(result);
	}

	// Comparison
	if (op_type == TOKEN_EQUAL_EQUAL || op_type == TOKEN_BANG_EQUAL) {
		int is_equal;
		if (left->type != right->type) {
			is_equal = 0;
		} else {
			switch (left->type) {
				case VAL_NUMBER:
					is_equal = left->as.number == right->as.number;
					break;

				case VAL_BOOL:
					is_equal = left->as.boolean == right->as.boolean;
					break;

				case VAL_STRING:
					is_equal = strcmp(left->as.string, right->as.string) == 0;
					break;

				case VAL_NIL:
					is_equal = 1; // nil == nil
					break;

				default:
					is_equal = 0;
					break;
			}
		}
		value_t *result = malloc(sizeof(value_t));
		result->type = VAL_BOOL;
		result->as.boolean = op_type == TOKEN_EQUAL_EQUAL ? is_equal : !is_equal;
		free_val(left);
		free_val(right);
		return result;
	}

	// Number Comparison
	if (left->type == VAL_NUMBER && right->type == VAL_NUMBER) {
		value_t *result = malloc(sizeof(value_t));
		result->type = VAL_BOOL;
		switch (op_type) {
			case TOKEN_GREATER:
				result->as.boolean = left->as.number > right->as.number;
				free_val(left);
				free_val(right);
				return result;

			case TOKEN_GREATER_EQUAL:
				result->as.boolean = left->as.number >= right->as.number;
				free_val(left);
				free_val(right);
				return result;

			case TOKEN_LESS:
				result->as.boolean = left->as.number < right->as.number;
				free_val(left);
				free_val(right);
				return result;

			case TOKEN_LESS_EQUAL:
				result->as.boolean = left->as.number <= right->as.number;
				free_val(left);
				free_val(right);
				return result;

			default: break;
		}
	}

	// String concatenation
	if (left->type == VAL_STRING && right->type == VAL_STRING) {
		if (op_type == TOKEN_PLUS) {
			value_t *result = malloc(sizeof(value_t));
			result->type = VAL_STRING;
			size_t left_len = strlen(left->as.string);
			size_t right_len = strlen(right->as.string);
			result->as.string = malloc(left_len + right_len + 1);
			strcpy(result->as.string, left->as.string);
			strcat(result->as.string, right->as.string);
			free_val(left);
			free_val(right);
			return result;
		}
	}

	// String/number comparisons
	if ((left->type == VAL_STRING && right->type == VAL_NUMBER) ||
			(left->type == VAL_NUMBER && right->type == VAL_STRING)) {
		runtime_error("Operands must be numbers.", expr->line);
	}

	runtime_error("Operands must be two numbers or two strings.", expr->line);
	value_t *val = malloc(sizeof(value_t));
	val->type = VAL_NIL;
	free_val(left);
	free_val(right);
	return val;
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

value_t *visit_unary(expr_t *expr, ht_t *env)
{
	value_t *operand = evaluate(expr->as.unary.right, env);

	if (expr->as.unary.operator.type == TOKEN_MINUS) {
		if (operand->type == VAL_NUMBER) {
			value_t *result = malloc(sizeof(value_t));
			result->type = VAL_NUMBER;
			result->as.number = -operand->as.number;
			free_val(operand);
			return result;
		} else {
			runtime_error("Operand must be a number.", expr->line);
		}
	} else if (expr->as.unary.operator.type == TOKEN_BANG) {
		value_t *result = malloc(sizeof(value_t));
		result->type = VAL_BOOL;
		result->as.boolean = !is_truthy(operand);
		free_val(operand);

		return result;
	}

	value_t *val = malloc(sizeof(value_t));
	val->type = VAL_NIL;
	free_val(operand);
	return val;
}

value_t *visit_variable(expr_t *expr, ht_t *env)
{
	value_t *val = ht_get(env, &expr->as.variable.name, 1);
	if (val) {
		return val;
	} else {
		val = malloc(sizeof(value_t));
		val->type = VAL_NIL;
		return val;
	}
}

value_t *visit_assign(expr_t *expr, ht_t *env)
{
	value_t *value = evaluate(expr->as.assign.value, env);
	ht_assign(env, &expr->as.assign.name->as.variable.name, value);
    return value;
}

value_t *visit_logical(expr_t *expr, ht_t *env)
{
	value_t *left = evaluate(expr->as.logical.left, env);

    if (expr->as.logical.operator.type == TOKEN_OR) {
      if (is_truthy(left))
		  return left;
    } else {
      if (!is_truthy(left))
		  return left;
    }

    return evaluate(expr->as.logical.right, env);
}

void val_add(val_array_t *array, value_t *expr)
{
	if (array->length == array->capacity) {
		array->capacity *= 2;
		array->arguments = realloc(array->arguments, array->capacity * sizeof(expr_t *));
	}
	array->arguments[array->length++] = expr;
}

void free_vals(val_array_t *array)
{
	for (int i = 0; i < array->length; i++) {
		free_val(array->arguments[i]);
	}
	free(array->arguments);
	free(array);
}

value_t *visit_call(expr_t *expr, ht_t *env)
{
	value_t *callee = evaluate(expr->as.call.callee, env);
	if (callee->type != VAL_FN) {
		runtime_error("Can only call functions and classes.", expr->line);
	}

	val_array_t *arguments = malloc(sizeof(val_array_t));
	arguments->arguments = malloc(DEFAULT_ARGS_SIZE * sizeof(value_t *));
	arguments->length = 0;
	arguments->capacity = DEFAULT_ARGS_SIZE;
	
	for (int i = 0; i < expr->as.call.args->length; i++) {
		value_t *val = evaluate(expr->as.call.args->arguments[i], env);
		val_add(arguments, val);
	}

	if (arguments->length != callee->as.function->arity) {
		char err[512];
		snprintf(err, 512, "Expected %d arguments but got %d.", callee->as.function->arity, arguments->length);
		runtime_error(err, expr->line);
    }
    value_t *res = callee->as.function->call(callee->as.function, arguments, callee->as.function->env);
	free_vals(arguments);
	free_val(callee);
	return res;
}

value_t *evaluate(expr_t *expr, ht_t *env)
{
	if (!expr) {
		value_t *nil = malloc(sizeof(value_t));
		nil->type = VAL_NIL;
		return nil;
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
		case EXPR_CALL:
			return visit_call(expr, env);
		default:
			exit(65);
			break;
	}
}

void print_value(value_t *value)
{
	if (!value) {
		printf("nil\n");
		return;
	}

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

		case VAL_FN:
			if (value->as.function->type == FN_NATIVE) {
				printf("<native fn>\n");
			} else {
				printf("<fn %s>\n", value->as.function->stmt->as.function.name.value);
			}
			break;

		default:
			break;
	}
}

void evaluate_statements(stmt_array_t *array, ht_t *env, return_state_t *state)
{
	for (int i = 0; i < array->length; i++) {
		evaluate_statement(array->statements[i], env, state);
	}
}

void evaluate_block(stmt_array_t *array, ht_t *cur_env, ht_t *scope_env, return_state_t *state)
{
	ht_t *previous = cur_env;
	cur_env = scope_env;
	evaluate_statements(array, cur_env, state);
/* 	ht_free(scope_env); */
	cur_env = previous;
}

value_t *_clock(fn_t *fn, val_array_t *arguments, ht_t *env)
{
	value_t *val = malloc(sizeof(value_t));
	val->type = VAL_NUMBER;
	val->as.number = time(NULL);
	return val;
}

value_t *_call(fn_t *fn, val_array_t *arguments, ht_t *env)
{
	ht_t *fn_env = ht_init(fn->env);
	for (int i = 0; i < fn->stmt->as.function.params->length; i++) {
		ht_add(fn_env, fn->stmt->as.function.params->tokens[i].value, arguments->arguments[i]);
	}

	return_state_t state = { 0, NULL };

	evaluate_block(fn->stmt->as.function.body->as.block.statements, env, fn_env, &state);

/* 	ht_free(fn_env); */
	if (state.has_returned) {
		return state.value;
	}
    return NULL;
}

void evaluate_statement(stmt_t *stmt, ht_t *env, return_state_t *state)
{
	if (state && state->has_returned)
		return;
	switch (stmt->type) {
		case STMT_IF:
			if (is_truthy(evaluate(stmt->as._if.condition, env))) {
				evaluate_statement(stmt->as._if.then_branch, env, state);
			} else if (stmt->as._if.else_branch) {
				evaluate_statement(stmt->as._if.else_branch, env, state);
			}
			break;

		case STMT_PRINT:;
			value_t *val = evaluate(stmt->as.print.expression, env);
			print_value(val);
			free_val(val);
			break;

		case STMT_EXPR:;
			value_t *res = evaluate(stmt->as.expr.expression, env);
			free_val(res);
			break;

		case STMT_VAR: {
			value_t *value = malloc(sizeof(value_t));
			value->type = VAL_NIL;
			if (stmt->as.variable.initializer) {
				free(value);
				value = evaluate(stmt->as.variable.initializer, env);
			}
			ht_add(env, stmt->as.variable.name.value, value);
			free_val(value);
			break;
		}

		case STMT_BLOCK:;
			evaluate_block(stmt->as.block.statements, env, ht_init(env), state);
			break;

		case STMT_WHILE:;
			value_t *cond = evaluate(stmt->as._while.condition, env);
			while (is_truthy(cond)) {
				evaluate_statement(stmt->as._while.body, env, state);
				free_val(cond);
				if (state->has_returned) {
					return;
				}
				cond = evaluate(stmt->as._while.condition, env);
			}
			free_val(cond);
			break;

		case STMT_FUN:;
			fn_t *fn = malloc(sizeof(fn_t));
			fn->type = FN_CUSTOM;
			fn->arity = stmt->as.function.params->length;
			fn->env = env;
			fn->stmt = stmt;
			fn->call = _call;

			value_t *fn_val = malloc(sizeof(value_t));
			fn_val->type = VAL_FN;
			fn_val->as.function = fn;
			ht_add(env, stmt->as.function.name.value, fn_val);
			free_val(fn_val);
			break;
		
		case STMT_RETURN:;
			value_t *value = NULL;
			if (stmt->as._return.value) {
				value = evaluate(stmt->as._return.value, env);
			}
			state->has_returned = 1;
            state->value = value;

		default:
			break;
	}
}

void interpret(stmt_array_t *array)
{
	ht_t *env = ht_init(NULL);
	value_t *clock_fn = malloc(sizeof(value_t));
	clock_fn->type = VAL_FN;
	fn_t *fn = malloc(sizeof(fn_t));
	fn->type = FN_NATIVE;
	fn->arity = 0;
	/* Native function don't have body */
	fn->stmt = NULL;
	fn->call = _clock;
	clock_fn->as.function = fn;

	ht_add(env, "clock", clock_fn);

	return_state_t state = { 0, NULL };

	evaluate_statements(array, env, &state);
	ht_free(env);
	free(clock_fn);
	free(fn);
	free_statements(array);
}
