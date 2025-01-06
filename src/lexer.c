#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "lexer.h"

const keyword_map reserved_keywords[] = {
	{"and", TOKEN_AND}, {"class", TOKEN_CLASS}, {"else", TOKEN_ELSE},
	{"false", TOKEN_FALSE}, {"fun", TOKEN_FUN}, {"for", TOKEN_FOR},
	{"if", TOKEN_IF}, {"nil", TOKEN_NIL}, {"or", TOKEN_OR},
	{"print", TOKEN_PRINT}, {"return", TOKEN_RETURN},
	{"super", TOKEN_SUPER}, {"this", TOKEN_THIS}, {"true", TOKEN_TRUE},
	{"var", TOKEN_VAR}, {"while", TOKEN_WHILE}
};

const keyword_map regular_tokens[] = {
	{"LEFT_PAREN", TOKEN_LEFT_PAREN}, {"RIGHT_PAREN", TOKEN_RIGHT_PAREN},
	{"LEFT_BRACE", TOKEN_LEFT_BRACE}, {"RIGHT_BRACE", TOKEN_RIGHT_BRACE},
	{"COMMA", TOKEN_COMMA}, {"DOT", TOKEN_DOT}, {"MINUS", TOKEN_MINUS},
	{"PLUS", TOKEN_PLUS}, {"SEMICOLON", TOKEN_SEMICOLON}, {"SLASH", TOKEN_SLASH},
	{"STAR", TOKEN_STAR}, {"BANG", TOKEN_BANG}, {"BANG_EQUAL", TOKEN_BANG_EQUAL},
	{"EQUAL", TOKEN_EQUAL}, {"EQUAL_EQUAL", TOKEN_EQUAL_EQUAL}, {"GREATER", TOKEN_GREATER},
	{"GREATER_EQUAL", TOKEN_GREATER_EQUAL}, {"LESS", TOKEN_LESS}, {"LESS_EQUAL", TOKEN_LESS_EQUAL},
	{"IDENTIFIER", TOKEN_IDENTIFIER}, {"STRING", TOKEN_STRING}, {"NUMBER", TOKEN_NUMBER},
	{"AND", TOKEN_AND}, {"CLASS", TOKEN_CLASS}, {"ELSE", TOKEN_ELSE}, {"FALSE", TOKEN_FALSE},
	{"FUN", TOKEN_FUN}, {"FOR", TOKEN_FOR}, {"IF", TOKEN_IF}, {"NIL", TOKEN_NIL},
	{"OR", TOKEN_OR}, {"PRINT", TOKEN_PRINT}, {"RETURN", TOKEN_RETURN},
	{"SUPER", TOKEN_SUPER}, {"THIS", TOKEN_THIS}, {"TRUE", TOKEN_TRUE},
	{"VAR", TOKEN_VAR}, {"WHILE", TOKEN_WHILE}, {"END_OF_FILE", TOKEN_EOF}
};

char *read_source(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Error reading file: %s\n", filename);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	rewind(file);

	char *source = malloc(file_size + 1);
	if (source == NULL) {
		fprintf(stderr, "Memory allocation failed\n");
		fclose(file);
		return NULL;
	}

	size_t bytes_read = fread(source, 1, file_size, file);
	if (bytes_read < file_size) {
		fprintf(stderr, "Error reading file contents\n");
		free(source);
		fclose(file);
		return NULL;
	}

	source[file_size] = '\0';
	fclose(file);

	return source;
}

int is_digit(char c)
{
	return c >= '0' && c <= '9';
}

int is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

void token_add(array_t *array, token_t token)
{
	if (array->length == array->capacity) {
		array->capacity *= 2;
		array->tokens = realloc(array->tokens, array->capacity * sizeof(token_t));
	}
	array->tokens[array->length++] = token;
}

token_t token_gen(token_type_t type, char *value, int line)
{
	token_t token;
	token.type = type;
	char *val = strdup(value);
	token.value = val;
	token.line = line;
	return token;
}

char *type_str(token_type_t type)
{
	for (int i = 0; i < sizeof(regular_tokens) / sizeof(regular_tokens[0]); i++) {
		if (regular_tokens[i].token_type == type) {
			return regular_tokens[i].keyword;
		}
	}
	return NULL;
}

void print_tokens(token_t *tokens)
{
	for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
		token_t token = tokens[i];
		if (token.type == TOKEN_STRING) {
			printf("STRING \"%s\" %s\n", token.value, token.value);
		} else if (token.type == TOKEN_NUMBER) {
			double value = strtod(token.value, NULL);
			if (value == (int) value) {
				printf("NUMBER %s %d.0\n", token.value, (int) value); 
			} else {
				printf("NUMBER %s %g\n", token.value, value); 
			}
		} else {
			printf("%s %s null\n", type_str(token.type), token.value);
		}
	}
	printf("EOF  null\n");
}

void free_array(array_t *array)
{
	for (int i = 0; i < array->length; i++) {
		if (array->tokens[i].value) {
			free(array->tokens[i].value);
		}
	}
	free(array->tokens);
	free(array);
}

array_t *tokenize(char *filename)
{
	array_t *tokens = malloc(sizeof(array_t));
	tokens->tokens = malloc(DEFAULT_TOKENS_SIZE * sizeof(token_t));
	tokens->length = 0;
	tokens->capacity = DEFAULT_TOKENS_SIZE;

	char *source = read_source(filename);
	int line = 1;
	size_t source_len = strlen(source);
	if (source_len > 0) {
		for (int i = 0; i < source_len; i++) {
			switch (source[i]) {
				case '(': 
					token_add(tokens, token_gen(TOKEN_LEFT_PAREN, "(", line));
					break;
				case ')':
					token_add(tokens, token_gen(TOKEN_RIGHT_PAREN, ")", line));
					break;
				case '{':
					token_add(tokens, token_gen(TOKEN_LEFT_BRACE, "{", line));
					break;
				case '}':
					token_add(tokens, token_gen(TOKEN_RIGHT_BRACE, "}", line));
					break;
				case '*':
					token_add(tokens, token_gen(TOKEN_STAR, "*", line));
					break;
				case '.':
					token_add(tokens, token_gen(TOKEN_DOT, ".", line));
					break;
				case ',':
					token_add(tokens, token_gen(TOKEN_COMMA, ",", line));
					break;
				case '+':
					token_add(tokens, token_gen(TOKEN_PLUS, "+", line));
					break;
				case '-':
					token_add(tokens, token_gen(TOKEN_MINUS, "-", line));
					break;
				case ';':
					token_add(tokens, token_gen(TOKEN_SEMICOLON, ";", line));
					break;
				case '=':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(TOKEN_EQUAL_EQUAL, "==", line));
						i++;
					} else {
						token_add(tokens, token_gen(TOKEN_EQUAL, "=", line));
					}
					break;
				case '!':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(TOKEN_BANG_EQUAL, "!=", line));
						i++;
					} else {
						token_add(tokens, token_gen(TOKEN_BANG, "!", line));
					}
					break;
				case '>':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(TOKEN_GREATER_EQUAL, ">=", line));
						i++;
					} else {
						token_add(tokens, token_gen(TOKEN_GREATER, ">", line));
					}
					break;
				case '<':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(TOKEN_LESS_EQUAL, "<=", line));
						i++;
					} else {
						token_add(tokens, token_gen(TOKEN_LESS, "<", line));
					}
					break;
				case '/':
					if (source[i + 1] == '/') {
						i += 2; // Skip both forward slashes
						while (i < strlen(source) && source[i] != '\n') {
							i++;
						}
						i--;
					} else {
						token_add(tokens, token_gen(TOKEN_SLASH, "/", line));
					}
					break;
				case ' ':
					break;
				case '\t':
					break;
				case '\n':
					line++;
					break;
				case '"':
					i++;
					int str_start = i;
					while (source[i] != '"' && source[i] != '\0') {
						if (source[i] == '\n')
							line++;
						i++;
					}
					if (source[i] == '\0') {
						fprintf(stderr, "[line %d] Error: Unterminated string.\n", line);
						errno = 65;
					} else {
						int len = i - str_start;
						char str[len + 1];
						strncpy(str, &source[str_start], len);
						str[len] = 0;
						token_add(tokens, token_gen(TOKEN_STRING, str, line));
					}
					break;

				default:
					if (is_alpha(source[i])) {
						int id_start = i;
						while (source[i] == '_' || (source[i] >= 'a' && source[i] <= 'z') ||
								(source[i] >= 'A' && source[i] <= 'Z') ||
								(source[i] >= '0' && source[i] <= '9')) {
							i++;
						}
						int len = i - id_start;
						char id[len + 1];
						strncpy(id, &source[i-len], len);
						id[len] = 0;

						int found = 0;
						for (int i = 0; i < sizeof(reserved_keywords) / sizeof(reserved_keywords[0]); i++) {
							if (strcmp(id, reserved_keywords[i].keyword) == 0) {
								char upper_id[len + 1];
								for (int i = 0; i < len; i++) {
									upper_id[i] = toupper(id[i]);
								}
								upper_id[len] = 0;
								token_add(tokens, token_gen(reserved_keywords[i].token_type, id, line));

								found = 1;
							}
						}
						if (!found) {
							token_add(tokens, token_gen(TOKEN_IDENTIFIER, id, line));
						}
						i--;
						break;

					} else if (is_digit(source[i])) {
						int int_start = i;
						while (isdigit(source[i]))
							i++;
						if (source[i] == '.' && isdigit(source[i + 1])) {
							i++;
							while (isdigit(source[i]))
								i++;
						}
						int len = i - int_start;
						char *iend = source + int_start;
						char integer[len + 1];
						strncpy(integer, iend, len);
						integer[len] = 0;
						token_add(tokens, token_gen(TOKEN_NUMBER, integer, line));
						i--;

					} else {
						fprintf(stderr, "[line %d] Error: Unexpected character: %c\n", line, source[i]);
						errno = 65;
					}
			}
		}
	}
	token_add(tokens, token_gen(TOKEN_EOF, "EOF", line));
	free(source);
	return tokens;
}
