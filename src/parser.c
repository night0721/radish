#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

int current = 0;
token_t *tokens;
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

expr_t *and(void)
{
	expr_t *expr = equality();

    while (check(TOKEN_AND)) {
		token_t *operator = peek();
		advance();
		expr_t *right = equality();
		expr = create_logical_expr(operator, expr, right);
    }

    return expr;
}

expr_t *or(void)
{
	expr_t *expr = and();

    while (check(TOKEN_OR)) {
		token_t *operator = peek();
		advance();
		expr_t *right = and();
		expr = create_logical_expr(operator, expr, right);
    }

    return expr;
}

expr_t *assignment(void)
{
	expr_t *expr = or();

	if (check(TOKEN_EQUAL)) {
		token_t *equals = peek();
		advance();
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
	if (stmt->type == STMT_PRINT) {
		free_expr(stmt->as.print.expression);
		free(stmt);
	} else if (stmt->type == STMT_EXPR) {
		free_expr(stmt->as.expr.expression);
		free(stmt);
	} else if (stmt->type == STMT_VAR) {
		free(stmt->as.variable.name.value);
		free_expr(stmt->as.variable.initializer);
		free(stmt);
	} else if (stmt->type == STMT_BLOCK) {
		free_statements(stmt->as.block.statements);
		free(stmt);
	} else if (stmt->type == STMT_IF) {
		free_expr(stmt->as._if.condition);
		free_statement(stmt->as._if.then_branch);
		free_statement(stmt->as._if.else_branch);
		free(stmt);
	} else if (stmt->type == STMT_WHILE) {
		free_expr(stmt->as._while.condition);
		free_statement(stmt->as._while.body);
		free(stmt);
	}
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
    if (check(TOKEN_SEMICOLON)) {
		advance();
    } else if (check(TOKEN_VAR)) {
		advance();
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
    if (check(TOKEN_ELSE)) {
		advance();
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
	if (check(TOKEN_FOR)) {
		advance();
		return for_stmt();
	}
	if (check(TOKEN_IF)) {
		advance();
		return if_stmt();
	}
	if (check(TOKEN_PRINT)) {
		advance();
		return print_stmt();
	}
	if (check(TOKEN_WHILE)) {
		advance();
		return while_stmt();
	}
	if (check(TOKEN_LEFT_BRACE)) {
		advance();
		return block_stmt();
	}
	return expression_stmt();
}

stmt_t *var_declaration(void)
{
	token_t *name = consume(TOKEN_IDENTIFIER, "Expect variable name.");

	expr_t *initializer = NULL;
	if (check(TOKEN_EQUAL)) {
		advance();
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
	if (check(TOKEN_VAR)) {
		advance();
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
