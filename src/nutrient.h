#pragma once

#include <stdint.h>
#include "nutrient_ffa.h"

typedef struct
{
    uint64_t root_offset;
    struct ffa *ffa;
} nutrient_tree;

nutrient_tree *nutrient_create(const char *filename);
nutrient_tree *nutrient_open(const char *filename);
int nutrient_sync(nutrient_tree *);
int nutrient_close(nutrient_tree *);

int nutrient_find(nutrient_tree * t, const uint8_t *key, uint32_t key_len,
                  const uint8_t **value, uint32_t * value_len);
int nutrient_insert(nutrient_tree * t, const uint8_t *key, uint32_t key_len,
                    const uint8_t *value, uint32_t value_len);

int nutrient_delete(nutrient_tree * t, const uint8_t *key, uint32_t key_len, const uint8_t * value, uint32_t value_len);

void nutrient_clear(nutrient_tree * t);
int nutrient_allprefixed(nutrient_tree * t, const uint8_t *key, uint32_t key_len,
                         int (*handle) (const uint8_t *, uint32_t, const uint8_t *,
                                        uint32_t, void *), void *arg);

int nutrient_find_predecessor(nutrient_tree * t, const uint8_t *key, uint32_t key_len,
                                const uint8_t **prefix, uint32_t * prefix_len,
                                const uint8_t **value, uint32_t * value_len);

int nutrient_find_longest_prefix(nutrient_tree * t, const uint8_t *key, uint32_t key_len,
                                const uint8_t **prefix, uint32_t * prefix_len,
                                const uint8_t **value, uint32_t * value_len);

void print_tree(nutrient_tree *t);
