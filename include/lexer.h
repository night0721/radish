#ifndef LEXER_H
#define LEXER_H

#define DEFAULT_TOKENS_SIZE 512

typedef enum {
  // Single-character tokens
  LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
  COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

  // One or two character tokens
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,

  // Literals
  IDENTIFIER, STRING, NUMBER,

  // Keywords
  AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
  PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

  END_OF_FILE
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

array_t *tokenize(char *filename);
void print_tokens(token_t *tokens);
void free_array(array_t *array);

#endif
