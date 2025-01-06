#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "lexer.h"

const keyword_map reserved_keywords[] = {
	{"and", AND}, {"class", CLASS}, {"else", ELSE},
	{"false", FALSE}, {"fun", FUN}, {"for", FOR},
	{"if", IF}, {"nil", NIL}, {"or", OR},
	{"print", PRINT}, {"return", RETURN},
	{"super", SUPER}, {"this", THIS}, {"true", TRUE},
	{"var", VAR}, {"while", WHILE}
};

const keyword_map regular_tokens[] = {
	{"LEFT_PAREN", LEFT_PAREN}, {"RIGHT_PAREN", RIGHT_PAREN},
	{"LEFT_BRACE", LEFT_BRACE}, {"RIGHT_BRACE", RIGHT_BRACE},
	{"COMMA", COMMA}, {"DOT", DOT}, {"MINUS", MINUS},
	{"PLUS", PLUS}, {"SEMICOLON", SEMICOLON}, {"SLASH", SLASH},
	{"STAR", STAR}, {"BANG", BANG}, {"BANG_EQUAL", BANG_EQUAL},
	{"EQUAL", EQUAL}, {"EQUAL_EQUAL", EQUAL_EQUAL}, {"GREATER", GREATER},
	{"GREATER_EQUAL", GREATER_EQUAL}, {"LESS", LESS}, {"LESS_EQUAL", LESS_EQUAL},
	{"IDENTIFIER", IDENTIFIER}, {"STRING", STRING}, {"NUMBER", NUMBER},
	{"AND", AND}, {"CLASS", CLASS}, {"ELSE", ELSE}, {"FALSE", FALSE},
	{"FUN", FUN}, {"FOR", FOR}, {"IF", IF}, {"NIL", NIL},
	{"OR", OR}, {"PRINT", PRINT}, {"RETURN", RETURN},
	{"SUPER", SUPER}, {"THIS", THIS}, {"TRUE", TRUE},
	{"VAR", VAR}, {"WHILE", WHILE}, {"END_OF_FILE", END_OF_FILE}
};

char *read_source(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "rd: Error reading file: %s\n", filename);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	rewind(file);

	char *source = malloc(file_size + 1);
	if (source == NULL) {
		fprintf(stderr, "rd: Error allocating memory\n");
		fclose(file);
		return NULL;
	}

	size_t bytes_read = fread(source, 1, file_size, file);
	if (bytes_read < file_size) {
		fprintf(stderr, "rd: Error reading file contents\n");
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
	for (int i = 0; tokens[i].type != END_OF_FILE; i++) {
		token_t token = tokens[i];
		if (token.type == STRING) {
			printf("STRING \"%s\" %s\n", token.value, token.value);
		} else if (token.type == NUMBER) {
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
	if (!source) {
		return NULL;
	}
	int line = 1;
	size_t source_len = strlen(source);
	if (source_len > 0) {
		for (int i = 0; i < source_len; i++) {
			switch (source[i]) {
				case '(': 
					token_add(tokens, token_gen(LEFT_PAREN, "(", line));
					break;
				case ')':
					token_add(tokens, token_gen(RIGHT_PAREN, ")", line));
					break;
				case '{':
					token_add(tokens, token_gen(LEFT_BRACE, "{", line));
					break;
				case '}':
					token_add(tokens, token_gen(RIGHT_BRACE, "}", line));
					break;
				case '*':
					token_add(tokens, token_gen(STAR, "*", line));
					break;
				case '.':
					token_add(tokens, token_gen(DOT, ".", line));
					break;
				case ',':
					token_add(tokens, token_gen(COMMA, ",", line));
					break;
				case '+':
					token_add(tokens, token_gen(PLUS, "+", line));
					break;
				case '-':
					token_add(tokens, token_gen(MINUS, "-", line));
					break;
				case ';':
					token_add(tokens, token_gen(SEMICOLON, ";", line));
					break;
				case '=':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(EQUAL_EQUAL, "==", line));
						i++;
					} else {
						token_add(tokens, token_gen(EQUAL, "=", line));
					}
					break;
				case '!':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(BANG_EQUAL, "!=", line));
						i++;
					} else {
						token_add(tokens, token_gen(BANG, "!", line));
					}
					break;
				case '>':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(GREATER_EQUAL, ">=", line));
						i++;
					} else {
						token_add(tokens, token_gen(GREATER, ">", line));
					}
					break;
				case '<':
					if (source[i + 1] == '=') {
						token_add(tokens, token_gen(LESS_EQUAL, "<=", line));
						i++;
					} else {
						token_add(tokens, token_gen(LESS, "<", line));
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
						token_add(tokens, token_gen(SLASH, "/", line));
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
						token_add(tokens, token_gen(STRING, str, line));
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
							token_add(tokens, token_gen(IDENTIFIER, id, line));
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
						token_add(tokens, token_gen(NUMBER, integer, line));
						i--;

					} else {
						fprintf(stderr, "[line %d] Error: Unexpected character: %c\n", line, source[i]);
						errno = 65;
					}
			}
		}
	}
	token_add(tokens, token_gen(END_OF_FILE, "EOF", line));
	free(source);
	return tokens;
}
