#pragma once

#include <stdint.h>

/*
** Create a new nutrient DB file
**
** @param filename  The filename for the new DB file
** @return a handle for adding/removing from the DB
*/
struct nutrient_tree *nutrient_create(const char *filename);

/*
** Open an existing nutrient DB file
**
** @param filename  The filename for the new DB file
** @return a handle for adding/removing from the DB
*/
struct nutrient_tree *nutrient_open(const char *filename);


int nutrient_sync(struct nutrient_tree *);
int nutrient_close(struct nutrient_tree *);

int nutrient_insert(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
		        const uint8_t * value, uint32_t value_len);

int nutrient_delete(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len, const uint8_t * value, uint32_t value_len);

void nutrient_clear(struct nutrient_tree * t);


int nutrient_find(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
		      const uint8_t ** value, uint32_t * value_len);

int nutrient_find_predecessor(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
			     const uint8_t ** prefix, uint32_t * prefix_len,
			      const uint8_t ** value, uint32_t * value_len);

int nutrient_find_longest_prefix(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
			     const uint8_t ** prefix, uint32_t * prefix_len,
			      const uint8_t ** value, uint32_t * value_len);

int nutrient_allprefixed(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
	          int (*handle) (const uint8_t *, uint32_t, const uint8_t *,
				     uint32_t, void *), void *arg);

#ifdef DEBUG
void print_tree(struct nutrient_tree * t);
#endif
