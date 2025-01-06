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
		fprintf(stderr, "Usage: rd tokenize|parse|evaluate <filename>\n");
		return 1;
	}

	const char *command = argv[1];
	
	if (!strcmp(command, "tokenize")) {
		array_t *array = tokenize(argv[2]);
		if (array) {
			print_tokens(array->tokens);
			free_array(array);
		}
	} else if (!strcmp(command, "parse")) {
		array_t *array = tokenize(argv[2]);
 		expr_t *expr = parse(array->tokens);
		if (errno != 65) {
			print_ast(expr);
			printf("\n");
		}
		free_array(array);
		free_expr(expr);
	} else if (!strcmp(command, "evaluate")) {
		array_t *array = tokenize(argv[2]);
 		expr_t *expr = parse(array->tokens);
		value_t val = evaluate(expr);
		print_value(&val);
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
