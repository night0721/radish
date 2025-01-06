#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ast.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: rd tokenize|parse|evaluate|run <filename>\n");
		return 1;
	}

	const char *command = argv[1];

	array_t *array = tokenize(argv[2]);
	if (!array) {
		return 1;
	}
	if (!strcmp(command, "tokenize")) {
		print_tokens(array->tokens);
		free_array(array);
	} else if (!strcmp(command, "parse")) {
		expr_t *expr = parse_expr(array->tokens);
		if (errno != 65) {
			print_ast(expr);
			printf("\n");
		}
		free_array(array);
		free_expr(expr);
	} else if (!strcmp(command, "evaluate")) {
		expr_t *expr = parse_expr(array->tokens);
		value_t val = evaluate(expr);
		print_value(&val);
		free_array(array);
		free_expr(expr);
	} else if (!strcmp(command, "run")) {
		stmt_array_t *stmts = parse(array->tokens);
		if (errno != 65) {
			print_statements(stmts);
			free_array(array);
			free_statements(stmts);
		}
	} else {
		fprintf(stderr, "Unknown command: %s\n", command);
		return 1;
	}
	if (errno == 65) {
		return 65;
	} else if (errno == 70) {
		return 70;
	}
	return 0;
}
