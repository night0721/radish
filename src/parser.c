#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ast.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

int current = 0;
token_t *tokens;
void free_args(arg_array_t *array);
expr_t *expression(void);
stmt_t *expression_stmt(void);
stmt_t *statement(void);
stmt_t *var_declaration(void);
stmt_t *declaration(void);
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
	exit(65);
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
			free(expr->as.literal.value);
			free(expr);
			break;
	
		case EXPR_VARIABLE:
			free(expr->as.variable.name.value);
			free(expr);
			break;

		case EXPR_ASSIGN:
			free_expr(expr->as.assign.name);
			free_expr(expr->as.assign.value);
			free(expr);
			break;

		case EXPR_LOGICAL:
			free(expr->as.logical.operator.value);
			free_expr(expr->as.logical.left);
			free_expr(expr->as.logical.right);
			free(expr);
			break;

		case EXPR_CALL:
			free_args(expr->as.call.args);
			free_expr(expr->as.call.callee);
			free(expr->as.call.paren.value);
			free(expr);
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

int match(token_type_t type)
{
	if (check(type)) {
		advance();
		return 1;
	} else {
		return 0;
	}
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
	if (match(TOKEN_FALSE) || match(TOKEN_TRUE) || match(TOKEN_NIL) ||
			match(TOKEN_NUMBER) || match(TOKEN_STRING)) {
		token_t *tok = previous();
		return create_literal_expr(tok);
	}

	if (match(TOKEN_IDENTIFIER)) {
		token_t *tok = previous();
		return create_variable_expr(tok);
	}

	if (match(TOKEN_LEFT_PAREN)) {
		expr_t *expr = expression();
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
		return create_grouping_expr(expr);
	}
	error(peek(), "Expect expression.");
	return NULL;
}

void arg_add(arg_array_t *array, expr_t *expr)
{
	if (array->length == array->capacity) {
		array->capacity *= 2;
		array->arguments = realloc(array->arguments, array->capacity * sizeof(expr_t *));
	}
	array->arguments[array->length++] = expr;
}

void free_args(arg_array_t *array)
{
	for (int i = 0; i < array->length; i++) {
		free_expr(array->arguments[i]);
	}
	free(array->arguments);
	free(array);
}

expr_t *call(void)
{
    expr_t *expr = primary();
	while (1) {
		if (match(TOKEN_LEFT_PAREN)) {
			arg_array_t *args = malloc(sizeof(arg_array_t));
			args->arguments = malloc(DEFAULT_ARGS_SIZE * sizeof(expr_t *));
			args->length = 0;
			args->capacity = DEFAULT_ARGS_SIZE;
			if (!check(TOKEN_RIGHT_PAREN)) {
				do {
					if (args->length >= 255) {
						error(peek(), "Can't have more than 255 arguments.");
					}
					arg_add(args, expression());
				} while (match(TOKEN_COMMA));
			}
			token_t *paren = consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
			expr = create_call_expr(expr, paren, args);
		} else {
			break;
		}
    }
	return expr;
}

expr_t *unary(void)
{
	if (match(TOKEN_BANG) || match(TOKEN_MINUS)) {
		token_t *operator = previous();
		expr_t *right = unary();
		return create_unary_expr(operator, right);
	}

	return call();
}

expr_t *factor(void)
{
	expr_t *expr = unary();

	while (match(TOKEN_SLASH) || match(TOKEN_STAR)) {
		token_t *operator = previous();
		expr_t *right = unary();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *term(void)
{
	expr_t *expr = factor();

	while (match(TOKEN_MINUS) || match(TOKEN_PLUS)) {
		token_t *operator = previous();
		expr_t *right = factor();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *comparison(void)
{
	expr_t *expr = term();

	while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL) || match(TOKEN_LESS)
			|| match(TOKEN_LESS_EQUAL)) {
		token_t *operator = previous();
		expr_t *right = term();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *equality(void)
{
	expr_t *expr = comparison();

	while (match(TOKEN_BANG_EQUAL) || match(TOKEN_EQUAL_EQUAL)) {
		token_t *operator = previous();
		expr_t *right = comparison();
		expr = create_binary_expr(operator, expr, right);
	}

	return expr;
}

expr_t *and(void)
{
	expr_t *expr = equality();

    while (match(TOKEN_AND)) {
		token_t *operator = previous();
		expr_t *right = equality();
		expr = create_logical_expr(operator, expr, right);
    }

    return expr;
}

expr_t *or(void)
{
	expr_t *expr = and();

    while (match(TOKEN_OR)) {
		token_t *operator = previous();
		expr_t *right = and();
		expr = create_logical_expr(operator, expr, right);
    }

    return expr;
}

expr_t *assignment(void)
{
	expr_t *expr = or();

	if (match(TOKEN_EQUAL)) {
		token_t *equals = previous();
		expr_t *value = assignment();

		if (expr->type == EXPR_VARIABLE) {
			return create_assign_expr(expr, value);
		}
		error(equals, "Invalid assignment target.");
	}

    return expr;
}

expr_t *expression(void)
{
	return assignment();
}

void stmt_add(stmt_array_t *array, stmt_t *stmt)
{
	if (array->length == array->capacity) {
		array->capacity *= 2;
		array->statements = realloc(array->statements, array->capacity * sizeof(stmt_t *));
	}
	array->statements[array->length++] = stmt;
}

void free_statement(stmt_t *stmt)
{
	if (!stmt) {
		return;
	}
	switch (stmt->type) {
		case STMT_PRINT:
			free_expr(stmt->as.print.expression);
			break;
		case STMT_EXPR:
			free_expr(stmt->as.expr.expression);
			break;
		case STMT_VAR:
			free(stmt->as.variable.name.value);
			free_expr(stmt->as.variable.initializer);
			break;
		case STMT_BLOCK:
			free_statements(stmt->as.block.statements);
			break;
		case STMT_IF:
			free_expr(stmt->as._if.condition);
			free_statement(stmt->as._if.then_branch);
			free_statement(stmt->as._if.else_branch);
			break;
		case STMT_WHILE:
			free_expr(stmt->as._while.condition);
			free_statement(stmt->as._while.body);
			break;
		case STMT_FUN:
			free(stmt->as.function.name.value);
			free_array(stmt->as.function.params);
			free_statement(stmt->as.function.body);
			break;
		case STMT_RETURN:
			free(stmt->as._return.keyword.value);
			free_expr(stmt->as._return.value);
			break;
		default:
			break;
	}
	free(stmt);
}

void free_statements(stmt_array_t *array)
{
	for (int i = 0; i < array->length; i++) {
		free_statement(array->statements[i]);
	}
	free(array->statements);
	free(array);
}

stmt_t *for_stmt(void)
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	stmt_t *initializer = NULL;
    if (match(TOKEN_SEMICOLON)) {
    } else if (match(TOKEN_VAR)) {
		initializer = var_declaration();
    } else {
		initializer = expression_stmt();
	}

	expr_t *condition = NULL;
    if (!check(TOKEN_SEMICOLON)) {
		condition = expression();
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

	expr_t *increment = NULL;
    if (!check(TOKEN_RIGHT_PAREN)) {
		increment = expression();
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

	stmt_t *body = statement();
	if (increment) {
		stmt_t *body_incremented = malloc(sizeof(stmt_t));
		body_incremented->type = STMT_BLOCK;
		stmt_array_t *statements = malloc(sizeof(stmt_array_t));
		statements->statements = malloc(DEFAULT_STMTS_SIZE * sizeof(stmt_t));
		statements->length = 0;
		statements->capacity = DEFAULT_STMTS_SIZE;
		stmt_add(statements, body);
		body_incremented->as.block.statements = statements;

		stmt_t *stmt = malloc(sizeof(stmt_t));
		stmt->type = STMT_EXPR;
		stmt->as.expr.expression = increment;
		stmt_add(statements, stmt);
		body = body_incremented;
    }
	if (!condition) {
		token_t tok;
		tok.type = TOKEN_TRUE;
		tok.value = strdup("true");
		tok.line = -1;
		condition = create_literal_expr(&tok);
	}
	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_WHILE;
	stmt->as._while.condition = condition;
	stmt->as._while.body = body;

	body = stmt;

	if (initializer) {
		stmt_t *body_initialized = malloc(sizeof(stmt_t));
		body_initialized->type = STMT_BLOCK;
		stmt_array_t *statements = malloc(sizeof(stmt_array_t));
		statements->statements = malloc(DEFAULT_STMTS_SIZE * sizeof(stmt_t));
		statements->length = 0;
		statements->capacity = DEFAULT_STMTS_SIZE;
		stmt_add(statements, initializer);
		stmt_add(statements, body);
		body_initialized->as.block.statements = statements;
		body = body_initialized;
    }

    return body;
}

stmt_t *if_stmt(void)
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expr_t *cond = expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");
	stmt_t *then_branch = statement();
    stmt_t *else_branch = NULL;
    if (match(TOKEN_ELSE)) {
		else_branch = statement();
    }
	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_IF;
	stmt->as._if.condition = cond;
	stmt->as._if.then_branch = then_branch;
	stmt->as._if.else_branch = else_branch;
	return stmt;
}

stmt_t *print_stmt(void)
{
	expr_t *value = expression();
	consume(TOKEN_SEMICOLON, "Expect ; after value.");
	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_PRINT;
	stmt->as.print.expression = value;
	return stmt;
}

stmt_t *return_stmt(void)
{
	token_t *keyword = previous();
	expr_t *value = NULL;
	if (!check(TOKEN_SEMICOLON)) {
		value = expression();
    }

	consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_RETURN;
	stmt->as._return.keyword.type = keyword->type;
	stmt->as._return.keyword.value = strdup(keyword->value);
	stmt->as._return.keyword.line = keyword->line;
	stmt->as._return.value = value;
    return stmt;
}

stmt_t *while_stmt(void)
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expr_t *condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    stmt_t *body = statement();

	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_WHILE;
	stmt->as._while.condition = condition;
	stmt->as._while.body = body;
	return stmt;
}

stmt_t *block_stmt(void)
{
	stmt_array_t *statements = malloc(sizeof(stmt_array_t));
	statements->statements = malloc(DEFAULT_STMTS_SIZE * sizeof(stmt_t));
	statements->length = 0;
	statements->capacity = DEFAULT_STMTS_SIZE;


    while (!check(TOKEN_RIGHT_BRACE) && !end()) {
		stmt_add(statements, declaration());
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");

	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_BLOCK;
	stmt->as.block.statements = statements;
	return stmt;
}

stmt_t *expression_stmt(void)
{
	expr_t *expr = expression();
	consume(TOKEN_SEMICOLON, "Expect ; after expression.");
	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_EXPR;
	stmt->as.expr.expression = expr;
	return stmt;
}

stmt_t *statement(void)
{
	if (match(TOKEN_FOR)) {
		return for_stmt();
	}
	if (match(TOKEN_IF)) {
		return if_stmt();
	}
	if (match(TOKEN_PRINT)) {
		return print_stmt();
	}
	if (match(TOKEN_RETURN)) {
		return return_stmt();
	}
	if (match(TOKEN_WHILE)) {
		return while_stmt();
	}
	if (match(TOKEN_LEFT_BRACE)) {
		return block_stmt();
	}
	return expression_stmt();
}

stmt_t *function(char *kind)
{
	char err[512];
	snprintf(err, 512, "Expect %s name.", kind);
	token_t *name = consume(TOKEN_IDENTIFIER, err);
	snprintf(err, 512, "Expect '(' after %s name.", kind);
	consume(TOKEN_LEFT_PAREN, err);
	array_t *parameters = malloc(sizeof(array_t));
	parameters->tokens = malloc(DEFAULT_ARGS_SIZE * sizeof(token_t));
	parameters->length = 0;
	parameters->capacity = DEFAULT_ARGS_SIZE;

	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			if (parameters->length >= 255) {
				error(peek(), "Can't have more than 255 parameters.");
			}

			token_t *param = consume(TOKEN_IDENTIFIER, "Expect parameter name.");
			token_t param_cpy;
			memcpy(&param_cpy, param, sizeof(token_t));
			param_cpy.value = strdup(param->value);
			token_add(parameters, param_cpy);
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	snprintf(err, 512, "Expect '{' before %s body.", kind);
	consume(TOKEN_LEFT_BRACE, err);
	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_FUN;
	stmt->as.function.name.type = name->type;
	stmt->as.function.name.value = strdup(name->value);
	stmt->as.function.name.line = name->line;
	stmt->as.function.params = parameters;
	stmt->as.function.body = block_stmt();
	return stmt;
}

stmt_t *var_declaration(void)
{
	token_t *name = consume(TOKEN_IDENTIFIER, "Expect variable name.");

	expr_t *initializer = NULL;
	if (match(TOKEN_EQUAL)) {
		initializer = expression();
	}

	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

	stmt_t *stmt = malloc(sizeof(stmt_t));
	stmt->type = STMT_VAR;
	stmt->as.variable.name.type = name->type;
	stmt->as.variable.name.value = strdup(name->value);
	stmt->as.variable.name.line = name->line;
	stmt->as.variable.initializer = initializer;
	return stmt;
}

stmt_t *declaration(void)
{
	if (match(TOKEN_FUN)) {
		return function("function");
	}
	if (match(TOKEN_VAR)) {
		return var_declaration();
	}

	return statement();
}

stmt_array_t *parse(token_t *tks)
{
	tokens = tks;
	if (errno == 65) {
		return NULL;
	} else {
		stmt_array_t *statements = malloc(sizeof(stmt_array_t));
		statements->statements = malloc(DEFAULT_STMTS_SIZE * sizeof(stmt_t *));
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
