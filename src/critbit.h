#ifndef CRITBIT_H_
#define CRITBIT_H_

#include <stdint.h>

typedef struct
{
    void *root;
} critbit0_tree;

int critbit0_find(critbit0_tree * t, const char * key, uint32_t key_len, uint64_t * value);
int critbit0_insert(critbit0_tree * t, const char * key, uint32_t key_len, uint64_t value);
int critbit0_delete(critbit0_tree * t, const char * key, uint32_t key_len);
int critbit0_update(critbit0_tree * t, const char * key, uint32_t key_len, uint64_t old_value, uint64_t new_value);
void critbit0_clear(critbit0_tree * t);
int critbit0_allprefixed(critbit0_tree * t, const char *key, uint32_t key_len,
                         int (*handle) (const char *, uint32_t, uint64_t, void *), void *arg);
void critbit0_set_allocator(int (*func)(void **, size_t));
void critbit0_set_deallocator(void (*func)(void *, size_t));

#endif /* CRITBIT_H_ */
