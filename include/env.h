#ifndef ENV_H
#define ENV_H

#include "ast.h"
#include "lexer.h"

typedef struct ht_t {
	char *name;
	value_t value;
	struct ht_t *enclosing;
} ht_t;

#define DEFAULT_HT_SIZE 50

ht_t *ht_init(ht_t *env);
void ht_add(ht_t *ht, char *name, value_t value);
value_t *ht_get(ht_t *ht, token_t *name, int check_enclosing);
void ht_replace(ht_t *ht, char *name, value_t value);
void ht_assign(ht_t *ht, token_t *name, value_t value);
void ht_free(ht_t *ht);

#endif
