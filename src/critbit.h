#ifndef CRITBIT_H_
#define CRITBIT_H_

#include <stdint.h>

typedef struct
{
    void *root;
} critbit0_tree;

int critbit0_contains(critbit0_tree * t, const char * key, uint32_t key_len);
int critbit0_insert(critbit0_tree * t, const char * key, uint32_t key_len, void * value);
int critbit0_delete(critbit0_tree * t, const char * key, uint32_t key_len);
void critbit0_clear(critbit0_tree * t);
int critbit0_allprefixed(critbit0_tree * t, const char *key, uint32_t key_len,
                         int (*handle) (const char *, uint32_t, void *, void *), void *arg);

#endif /* CRITBIT_H_ */
