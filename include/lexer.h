#ifndef LEXER_H
#define LEXER_H

#define DEFAULT_TOKENS_SIZE 512

typedef enum {
  // Single-character tokens
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS, TOKEN_SEMICOLON,
  TOKEN_SLASH, TOKEN_STAR,

  // One or two character tokens
  TOKEN_BANG, TOKEN_BANG_EQUAL, TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_GREATER,
  TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL,

  // Literals
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

  // Keywords
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE, TOKEN_FUN, TOKEN_FOR,
  TOKEN_IF, TOKEN_NIL, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER,
  TOKEN_THIS, TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_EOF
} token_type_t;

typedef struct {
    char *keyword;
    token_type_t token_type;
} keyword_map;

typedef struct {
	token_type_t type;
	char *value;
	int line;
} token_t;

typedef struct {
    token_t *tokens;
    int length;
    int capacity;
} array_t;

void token_add(array_t *array, token_t token);
array_t *tokenize(char *filename);
void print_tokens(token_t *tokens);
void free_array(array_t *array);

#endif
