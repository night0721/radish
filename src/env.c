#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "interpreter.h"

void free_statement(stmt_t *stmt);

ht_t *ht_init(ht_t *env)
{	
	ht_t *ht = malloc(sizeof(ht_t) * DEFAULT_HT_SIZE);
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		ht[i].value.type = VAL_NIL;
		ht[i].name = NULL;
	}
	if (env) {
		ht->enclosing = env;	
	} else {
		ht->enclosing = NULL;
	}
	return ht;
}

unsigned int hash(char *key)
{
	unsigned int h = 0;
	for (; *key; key++)
		h = 31 * h + *key;
	return h;
}

void ht_add(ht_t *ht, char *name, value_t *value)
{
	unsigned int idx = hash(name) % DEFAULT_HT_SIZE;
	/* Linear probing for collision resolution */
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		int probe_idx = (idx + i) % DEFAULT_HT_SIZE;
		if (!ht[probe_idx].name) {
			ht[probe_idx].name = strdup(name);
			ht[probe_idx].value.type = value->type;
			if (value->type == VAL_STRING) {
				ht[probe_idx].value.as.string = strdup(value->as.string);
			} else if (value->type == VAL_FN) {
				ht[probe_idx].value.as.function = malloc(sizeof(fn_t));
				memcpy(ht[probe_idx].value.as.function, value->as.function, sizeof(fn_t));
			} else {
				ht[probe_idx].value.as = value->as;
			}
			return;
		} else {
			ht_replace(ht, name, value);
			return;
		}
	}
}

value_t *ht_get(ht_t *ht, token_t *name, int check_enclosing)
{
	if (!ht) {
		return NULL;
	}
	unsigned int idx = hash(name->value) % DEFAULT_HT_SIZE;
	/* Linear probing to search for the key */
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		int probe_idx = (idx + i) % DEFAULT_HT_SIZE;
		if (ht[probe_idx].name && !strcmp(ht[probe_idx].name, name->value)) {
			value_t *val = malloc(sizeof(value_t));
			memcpy(val, &ht[probe_idx].value, sizeof(value_t));
			if (val->type == VAL_STRING) {
				val->as.string = strdup(ht[probe_idx].value.as.string);
			} else if (val->type == VAL_FN) {
				val->as.function = malloc(sizeof(fn_t));
				memcpy(val->as.function, ht[probe_idx].value.as.function, sizeof(fn_t));
			}

			return val;
		}
	}
	if (check_enclosing) {
		if (ht->enclosing) {
			return ht_get(ht->enclosing, name, 1);
		}
	} else {
		return NULL;
	}

	char err[512];
	snprintf(err, 512, "Undefined variable '%s'.", name->value);
	runtime_error(err, name->line);
	return NULL;
}

void ht_replace(ht_t *ht, char *name, value_t *value)
{
	unsigned int idx = hash(name) % DEFAULT_HT_SIZE;

	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		int probe_idx = (idx + i) % DEFAULT_HT_SIZE;
		if (!ht[probe_idx].name) {
			ht_replace(ht->enclosing, name, value);
			break;
		}
		if (!strcmp(ht[probe_idx].name, name)) {
			if (ht[probe_idx].value.type == VAL_STRING) {
				free(ht[probe_idx].value.as.string);
			} else if (ht[probe_idx].value.type == VAL_FN) {
				free(ht[probe_idx].value.as.function);
			}
			ht[probe_idx].value.type = value->type;
			ht[probe_idx].value.as = value->as;
			if (value->type == VAL_STRING) {
				ht[probe_idx].value.as.string = strdup(value->as.string);
			} else if (value->type == VAL_FN) {
				ht[probe_idx].value.as.function = malloc(sizeof(fn_t));
				memcpy(ht[probe_idx].value.as.function, value->as.function, sizeof(fn_t));
			}
			return;
		}
	}
}

void ht_assign(ht_t *ht, token_t *name, value_t *value)
{
	value_t *val = ht_get(ht, name, 0);
	if (val) {
		ht_replace(ht, name->value, value);
		free_val(val);
		return;
	}
	if (ht->enclosing) {
		ht_assign(ht->enclosing, name, value);
		return;
	}
	char err[512];
	snprintf(err, 512, "Undefined variable '%s'.", name->value);
	runtime_error(err, name->line);
}

void ht_free(ht_t *ht)
{
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		if (ht[i].value.type != VAL_NIL) {
			free(ht[i].name);
			if (ht[i].value.type == VAL_STRING) {
				free(ht[i].value.as.string);
			} else if (ht[i].value.type == VAL_FN) {
				free(ht[i].value.as.function);
			}
		}
	}
	free(ht);
}

