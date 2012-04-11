#define _POSIX_C_SOURCE 200112
#define uint8 uint8_t
#define uint32 uint32_t

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <errno.h>

#include "critbit.h"

static int (*allocator)(void **, size_t, size_t) = posix_memalign;

typedef struct
{
    void *child[2];
    uint32 byte;
    uint8 otherbits;
} critbit0_node;

static void * _critbit0_find(critbit0_tree * t, const char * key, uint32 key_len)
{
    uint8 *p = t->root;
    uint32 found_len;

    if (!p)
        return NULL;

    while (1 & (intptr_t) p) {
        critbit0_node *q = (void *) (p - 1);
        uint8 c = 0;
        if (q->byte < key_len)
            c = key[q ->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;

        p = q->child[direction];
    }

    memcpy(&found_len, p, sizeof(uint32_t));

    if (found_len == key_len && 0 == memcmp(key, ((const char *) p) + sizeof(uint32) + sizeof(void *), key_len))
    {
        return p;
    }

    return NULL;
}

void * critbit0_find(critbit0_tree * t, const char * key, uint32 key_len)
{
    void * r = _critbit0_find(t, key, key_len);;

    if (!r)
        return NULL;

    memcpy(&r, ((const char *) r) + sizeof(uint32), sizeof(void *));

    return r;
}

int critbit0_update(critbit0_tree * t, const char * key, uint32_t key_len, void * old_value, void * new_value)
{

    return 0;
}

int critbit0_insert(critbit0_tree * t, const char * key, uint32 key_len, void * value)
{
    const uint8 *const ubytes = (void *) key;
    uint8 *p = t->root;

    if (!p) {
        char *x;
        int a = allocator((void **) &x, sizeof(void *), sizeof(uint32) + sizeof(value) + key_len);
        if (a)
            return 0;

        /* Copy the key length */
        memcpy(x, &key_len, sizeof(uint32));

        /* Copy the value */
        memcpy(x + sizeof(uint32), &value, sizeof(&value));

        /* Copy the key */
        memcpy(x + sizeof(uint32) + sizeof(value), key, key_len);

        t->root = x;
        return 2;
    }

    while (1 & (intptr_t) p) {
        critbit0_node *q = (void *) (p - 1);
        uint8 c = 0;
        if (q->byte < key_len)
            c = ubytes[q->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;

        p = q->child[direction];
    }

    uint32 newbyte;
    uint32 newotherbits;

    for (newbyte = 0; newbyte < key_len; ++newbyte) {
        if (p[sizeof(uint32) + sizeof(value) + newbyte] != ubytes[newbyte]) {
            newotherbits = p[sizeof(uint32) + sizeof(value) + newbyte] ^ ubytes[newbyte];
            goto different_byte_found;
        }
    }

    if (p[newbyte] != 0) {
        newotherbits = p[sizeof(uint32) + sizeof(value) + newbyte];
        goto different_byte_found;
    }
    return 1;

  different_byte_found:

    while (newotherbits & (newotherbits - 1))
        newotherbits &= newotherbits - 1;
    newotherbits ^= 255;
    uint8 c = p[sizeof(uint32) + sizeof(value) + newbyte];
    int newdirection = (1 + (newotherbits | c)) >> 8;

    critbit0_node *newnode;
    if (allocator
        ((void **) &newnode, sizeof(void *), sizeof(critbit0_node)))
        return 0;

    char *x;
    if (allocator((void **) &x, sizeof(void *), sizeof(uint32) + sizeof(value) + key_len)) {
        free(newnode);
        return 0;
    }

    memcpy(x, &key_len, sizeof(uint32));
    memcpy(x + sizeof(uint32), &value, sizeof(&value));
    memcpy(x + sizeof(uint32) + sizeof(value), ubytes, key_len);

    newnode->byte = newbyte;
    newnode->otherbits = newotherbits;
    newnode->child[1 - newdirection] = x;

    void **wherep = &t->root;
    for (;;) {
        uint8 *p = *wherep;
        if (!(1 & (intptr_t) p))
            break;
        critbit0_node *q = (void *) (p - 1);
        if (q->byte > newbyte)
            break;
        if (q->byte == newbyte && q->otherbits > newotherbits)
            break;
        uint8 c = 0;
        if (q->byte < key_len)
            c = ubytes[q->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;
        wherep = q->child + direction;
    }

    newnode->child[newdirection] = *wherep;
    *wherep = (void *) (1 + (char *) newnode);

    return 2;
}

int critbit0_delete(critbit0_tree * t, const char * key, uint32 key_len)
{
    const uint8 *ubytes = (void *) key;
    uint8 *p = t->root;
    void **wherep = &t->root;
    void **whereq = 0;
    critbit0_node *q = 0;
    int direction = 0;

    if (!p)
        return 0;

    while (1 & (intptr_t) p) {
        whereq = wherep;
        q = (void *) (p - 1);
        uint8 c = 0;
        if (q->byte < key_len)
            c = ubytes[q->byte];
        direction = (1 + (q->otherbits | c)) >> 8;
        wherep = q->child + direction;
        p = *wherep;
    }

    if (0 != memcmp(key, ((const char *) p) + sizeof(uint32) + sizeof(void *), key_len))
        return 0;
    free(p);

    if (!whereq) {
        t->root = 0;
        return 1;
    }

    *whereq = q->child[1 - direction];
    free(q);

    return 1;
}

static void traverse(void *top)
{
    uint8 *p = top;

    if (1 & (intptr_t) p) {
        critbit0_node *q = (void *) (p - 1);
        traverse(q->child[0]);
        traverse(q->child[1]);
        free(q);
    }
    else {
        free(p);
    }
}

void critbit0_clear(critbit0_tree * t)
{
    if (t->root)
        traverse(t->root);
    t->root = NULL;
}

static int
allprefixed_traverse(uint8 * top,
                     int (*handle) (const char *, uint32_t, void *, void *), void *arg)
{
    uint32_t key_len;
    void * value;

    if (1 & (intptr_t) top) {
        critbit0_node *q = (void *) (top - 1);
        for (int direction = 0; direction < 2; ++direction)
            switch (allprefixed_traverse(q->child[direction], handle, arg)) {
            case 1:
                break;
            case 0:
                return 0;
            default:
                return -1;
            }
        return 1;
    }

    memcpy(&key_len, top, sizeof(uint32_t));
    memcpy(&value, top + sizeof(uint32_t), sizeof(void *));

    return handle((const char *) top + sizeof(uint32_t) + sizeof(void *), key_len, value,  arg);     /*:27 */
}

int
critbit0_allprefixed(critbit0_tree * t, const char * prefix, uint32 prefix_len,
                     int (*handle) (const char *, uint32_t, void *, void *), void *arg)
{
    const uint8 *ubytes = (void *) prefix;
    uint8 *p = t->root;
    uint8 *top = p;

    if (!p)
        return 1;

    while (1 & (intptr_t) p) {
        critbit0_node *q = (void *) (p - 1);
        uint8 c = 0;
        if (q->byte < prefix_len)
            c = ubytes[q->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;
        p = q->child[direction];
        if (q->byte < prefix_len)
            top = p;
    }

    for (size_t i = 0; i < prefix_len; ++i) {
        if (p[i] != ubytes[i])
            return 1;
    }

    return allprefixed_traverse(top, handle, arg);
}
