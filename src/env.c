#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "interpreter.h"

ht_t *ht_init(void)
{
	ht_t *ht = malloc(sizeof(ht_t) * DEFAULT_HT_SIZE);
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		ht[i].name = NULL;
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

void ht_add(ht_t *ht, char *name, value_t value)
{
	unsigned int idx = hash(name) % DEFAULT_HT_SIZE;
	/* Linear probing for collision resolution */
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		int probe_idx = (idx + i) % DEFAULT_HT_SIZE;
		if (!ht[probe_idx].name) {
			ht[probe_idx].name = name;
			memcpy(&ht[probe_idx].value, &value, sizeof(value));
			return;
		} else {
			ht_replace(ht, name, value);
			return;
		}
	}
}

value_t *ht_get(ht_t *ht, token_t *name)
{
	unsigned int idx = hash(name->value) % DEFAULT_HT_SIZE;
	/* Linear probing to search for the key */
	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		int probe_idx = (idx + i) % DEFAULT_HT_SIZE;
		if (ht[probe_idx].name && !strcmp(ht[probe_idx].name, name->value))
			return &ht[probe_idx].value;
	}
	char err[512];
	snprintf(err, 512, "Undefined variable '%s'.", name->value);
	runtime_error(err, name->line);
	return NULL;
}

void ht_replace(ht_t *ht, char *name, value_t value)
{
	unsigned int idx = hash(name) % DEFAULT_HT_SIZE;

	for (int i = 0; i < DEFAULT_HT_SIZE; i++) {
		int probe_idx = (idx + i) % DEFAULT_HT_SIZE;
		if (!ht[probe_idx].name)
			break;
		if (!strcmp(ht[probe_idx].name, name)) {
			memcpy(&ht[probe_idx].value, &value, sizeof(value));
			return;
		}
	}
}

void ht_assign(ht_t *ht, token_t *name, value_t value)
{
	if (ht_get(ht, name)) {
		ht_replace(ht, name->value, value);
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
		}
	}
	free(ht);
}

