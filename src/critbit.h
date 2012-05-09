#pragma once

#include <stdint.h>

typedef struct
{
    void *root;
} critbit0_tree;

int critbit0_find(critbit0_tree * t, const char *key, uint32_t key_len,
                  const char ** value, uint32_t * value_len);
int critbit0_insert(critbit0_tree * t, const char *key, uint32_t key_len,
                    const char * value, uint32_t value_len);

int critbit0_delete(critbit0_tree * t, const char *key, uint32_t key_len);

void critbit0_clear(critbit0_tree * t);
int critbit0_allprefixed(critbit0_tree * t, const char *key, uint32_t key_len,
                         int (*handle) (const char *, uint32_t, const char *, uint32_t,
                                        void *), void *arg);
void critbit0_set_allocator(int (*func) (void **, size_t));
void critbit0_set_deallocator(void (*func) (void *, size_t));
