#pragma once

#include <stdint.h>
#include "nutrient_ffa.h"

typedef struct
{
    uint64_t root_offset;
    struct ffa *ffa;
} critbit0_tree;

critbit0_tree *critbit0_create(const char *filename);
critbit0_tree *critbit0_open(const char *filename);
int critbit0_sync(critbit0_tree *);
int critbit0_close(critbit0_tree *);

int critbit0_find(critbit0_tree * t, const uint8_t *key, uint32_t key_len,
                  const uint8_t **value, uint32_t * value_len);
int critbit0_insert(critbit0_tree * t, const uint8_t *key, uint32_t key_len,
                    const uint8_t *value, uint32_t value_len);

int critbit0_delete(critbit0_tree * t, const uint8_t *key, uint32_t key_len, const uint8_t * value, uint32_t value_len);

void critbit0_clear(critbit0_tree * t);
int critbit0_allprefixed(critbit0_tree * t, const uint8_t *key, uint32_t key_len,
                         int (*handle) (const uint8_t *, uint32_t, const uint8_t *,
                                        uint32_t, void *), void *arg);

int critbit0_find_predecessor(critbit0_tree * t, const uint8_t *key, uint32_t key_len,
                                const uint8_t **prefix, uint32_t * prefix_len,
                                const uint8_t **value, uint32_t * value_len);

int critbit0_find_longest_prefix(critbit0_tree * t, const uint8_t *key, uint32_t key_len,
                                const uint8_t **prefix, uint32_t * prefix_len,
                                const uint8_t **value, uint32_t * value_len);

void print_tree(critbit0_tree *t);
