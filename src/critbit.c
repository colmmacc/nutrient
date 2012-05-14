#define _POSIX_C_SOURCE 200112

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <errno.h>

#include "critbit.h"
#include "ffa.h"

typedef struct
{
    uint64_t child[2];
    uint32_t byte;
    uint8_t otherbits;
} critbit0_node;

critbit0_tree * critbit0_create(const char * filename)
{
    struct ffa * f;
    uint64_t r;
    critbit0_tree * tree;

    f = ffa_create(filename);
    if (f == NULL) {
        return NULL;
    }

    tree = malloc(sizeof(critbit0_tree));
    if (tree == NULL) {
        return NULL;
    }

    tree->root_offset = 0;
    tree->ffa = f;

    r = ffa_alloc(f, 8 + sizeof(uint64_t));
    if (r == FFA_ERROR)
    {   
        return NULL;
    }

    /* Copy our magic number in */
    memcpy(ffa_get_memory(f, r), "CrItBiT0", 8);

    /* Copy the offset of the root node */
    memcpy(ffa_get_memory(f, r + 8), &tree->root_offset, sizeof(uint64_t)); 

    /* Sync the file */
    ffa_sync(f);

    return tree;
}

static void *_critbit0_find(critbit0_tree * t, const char *key,
                            uint32_t key_len)
{
    uint64_t p = t->root_offset;
    uint32_t found_key_len;
    uint32_t found_value_len;

    if (!p)
        return NULL;

    while (1 & p) {
        critbit0_node *q = ffa_get_memory(t->ffa, p - 1);
        uint8_t c = 0;
        if (q->byte < key_len)
            c = key[q->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;

        p = q->child[direction];
    }

    memcpy(&found_key_len, ffa_get_memory(t->ffa, p), sizeof(uint32_t));
    memcpy(&found_value_len, ffa_get_memory(t->ffa, p + sizeof(uint32_t)), sizeof(uint32_t));

    if (found_key_len == key_len
        && 0 == memcmp(key,
                       ffa_get_memory(t->ffa, p + sizeof(uint32_t) + sizeof(uint32_t)),
                       key_len)) {
        return ffa_get_memory(t->ffa, p);
    }

    return NULL;
}

int critbit0_allocator(void **memory, size_t size, void * ctx)
{
    static void * base = NULL;
    static size_t total_size = 0;

    /* Only allocate aligned segments */
    if (size % (sizeof(void *))) {
        size += sizeof(void *) - (size % (sizeof(void *)));
    }

    /* Increase the total size */
    total_size += size;

    if (base == NULL) 
    {
        base = malloc(1024 * 1024 * 10);
    }

#if 0
    /* Allocate some memory */
    base = realloc(base, total_size);
    if (base == NULL)
    {
        return -1;
    }
#endif

    /* Set our pointer */
    *memory = ((char *) base) + (total_size - size);

    printf("p: %p\n", *memory);

    //return posix_memalign(memory, sizeof(void *), size);

    return 0;
}

static int (*allocator) (void **, size_t, void *) = critbit0_allocator;
static void * allocator_ctx;

void critbit0_set_allocator(int (*func) (void **, size_t, void *), void * ctx)
{
    allocator = func;
    allocator_ctx = ctx;
}

static void critbit0_deallocator(void *memory, size_t size, void * ctx)
{
    //free(memory);
}

static void (*deallocator) (void *, size_t size, void *) = critbit0_deallocator;
static void * deallocator_ctx;

void critbit0_set_deallocator(void (*func) (void *, size_t, void *), void * ctx)
{
    deallocator = func;
    deallocator_ctx = ctx;
}

int critbit0_find(critbit0_tree * t, const char *key, uint32_t key_len,
                  const char **value, uint32_t * value_len)
{
    void *r = _critbit0_find(t, key, key_len);;

    if (!r)
        return -1;

    /* Copy the value length */
    memcpy(value_len, (const char *) r + sizeof(uint32_t), sizeof(uint32_t));

    /* Point the value to the right data */
    *value = (const char *) r + sizeof(uint32_t) + sizeof(uint32_t) + key_len;

    return 0;
}

int critbit0_insert(critbit0_tree * t, const char *key, uint32_t key_len,
                    const char *value, uint32_t value_len)
{
    const uint8_t *const ubytes = (void *) key;
    uint64_t p = t->root_offset;

    /* If there's no root node, then this key is automatically
    ** the new root node.
    */
    if (!p) {
        uint64_t x = ffa_alloc(t->ffa, sizeof(key_len) + key_len + sizeof(value_len) + value_len);
        if (x == FFA_ERROR)
            return 0;

        /* Copy the key and value length */
        memcpy(ffa_get_memory(t->ffa, x), &key_len, sizeof(uint32_t));
        memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t)), &value_len, sizeof(uint32_t));

        /* Copy the key */
        memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t)), key, key_len);

        /* Copy the value */
        memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t) + key_len), value, value_len);

        /* Create a root node */
        t->root_offset = x;

        /* Update the saved copy of the root offset */
        memcpy(ffa_get_memory(t->ffa, 8), &t->root_offset, sizeof(uint64_t));

        return 2;
    }

    /* Keep decending into children, as long as the LSB is "1" */
    while (1 & p) {
        critbit0_node *q = ffa_get_memory(t->ffa, p - 1);
        uint8_t c = 0;
        if (q->byte < key_len)
            c = ubytes[q->byte];

        /* Pick the direction */
        const int direction = (1 + (q->otherbits | c)) >> 8;
        p = q->child[direction];
    }

    /* At this point, p is the offset of the closest match node to our key,
    ** possibly an exact match - if not, then a viable parent node 
    */

    uint32_t newbyte;
    uint32_t newotherbits;

    /* Compare the key to the node, until we find a difference */
    for (newbyte = 0; newbyte < key_len; ++newbyte) {
        if (((char *) ffa_get_memory(t->ffa, p + sizeof(uint32_t) + sizeof(uint32_t)))[newbyte] != ubytes[newbyte]) {
            newotherbits =
                ((char *) ffa_get_memory(t->ffa, p + sizeof(uint32_t) + sizeof(uint32_t)))[newbyte] ^ ubytes[newbyte];
            goto different_byte_found;
        }
    }

    /* The key is identical to p, right up to just before "newotherbits" */
    if (((char *) ffa_get_memory(t->ffa, p + sizeof(uint32_t) + sizeof(uint32_t)))[newbyte] != 0) {
        newotherbits = ((char *) ffa_get_memory(t->ffa, p + sizeof(uint32_t) + sizeof(uint32_t)))[newbyte];
        goto different_byte_found;
    }

    /* If we got to here, there's some kind of error */
    return 1;

  different_byte_found:

    /* TODO: wtf? */
    while (newotherbits & (newotherbits - 1))
        newotherbits &= newotherbits - 1;
    newotherbits ^= 255;

    /* What direction from the parent should we insert the key ? */
    uint8_t c = ((char *) ffa_get_memory(t->ffa, p + sizeof(uint32_t) + sizeof(uint32_t)))[newbyte];
    int newdirection = (1 + (newotherbits | c)) >> 8;

    /* Create the new node for key */
    uint64_t n = ffa_alloc(t->ffa, sizeof(critbit0_node));
    if (n == FFA_ERROR) {
        return 0;
    }

    critbit0_node *newnode = ffa_get_memory(t->ffa, n);

    /* Allocate space for its data */
    uint64_t x = ffa_alloc(t->ffa, sizeof(uint32_t) + sizeof(uint32_t) + key_len + value_len);
    if (x == FFA_ERROR) {
        return 0;
    }

    memcpy(ffa_get_memory(t->ffa, x), &key_len, sizeof(uint32_t));
    memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t)), &value_len, sizeof(uint32_t));
    memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t)), key, key_len);
    memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t) + key_len), value, value_len);

    newnode->byte = newbyte;
    newnode->otherbits = newotherbits;
    newnode->child[1 - newdirection] = x;

    uint64_t *wherep = &t->root_offset;
    for (;;) {
        uint64_t p = *wherep;
        if (!(1 & (intptr_t) p))
            break;
        critbit0_node *q = ffa_get_memory(t->ffa, p - 1);
        if (q->byte > newbyte)
            break;
        if (q->byte == newbyte && q->otherbits > newotherbits)
            break;
        uint8_t c = 0;
        if (q->byte < key_len)
            c = ubytes[q->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;
        wherep = &q->child[direction];
    }

    newnode->child[newdirection] = *wherep;
    *wherep = 1 + (intptr_t) n;

    if (wherep == &t->root_offset) {
        /* Update the saved copy of the root offset */
        memcpy(ffa_get_memory(t->ffa, 8), &t->root_offset, sizeof(uint64_t));
    }

    return 2;
}

int critbit0_delete(critbit0_tree * t, const char *key, uint32_t key_len)
{
    const uint8_t *ubytes = (void *) key;
    uint64_t p = t->root_offset;
    uint64_t *wherep = &t->root_offset;
    uint64_t *whereq = 0;
    critbit0_node *q = 0;
    int direction = 0;
    uint32_t found_key_len;
    uint32_t found_value_len;

    if (!p)
        return 0;

    while (1 & p) {
        whereq = wherep;
        q = ffa_get_memory(t->ffa, p - 1);
        uint8_t c = 0;
        if (q->byte < key_len)
            c = ubytes[q->byte];
        direction = (1 + (q->otherbits | c)) >> 8;
        wherep = q->child + direction;
        p = *wherep;
    }

    memcpy(&found_key_len, ffa_get_memory(t->ffa, p), sizeof(uint32_t));
    memcpy(&found_value_len, ffa_get_memory(t->ffa, p + sizeof(uint32_t)), sizeof(uint32_t));

    if (found_key_len != key_len
        || 0 != memcmp(key,
                       ffa_get_memory(t->ffa, p + sizeof(uint32_t) +
                       sizeof(uint32_t)), key_len))
        return 0;

    //deallocator(p, sizeof(uint32_t) + sizeof(uint32_t) + key_len + found_value_len);

    if (!whereq) {
        t->root_offset = 0;
        return 1;
    }

    *whereq = q->child[1 - direction];
    //deallocator(q, sizeof(critbit0_node));

    return 1;
}

static void traverse(critbit0_tree * t, uint64_t top)
{
    uint64_t p = top;

    if (1 & p) {
        critbit0_node *q = ffa_get_memory(t->ffa, p - 1);
        traverse(t, q->child[0]);
        traverse(t, q->child[1]);
        deallocator(t, p - 1, sizeof(critbit0_node));
    }
    else {
        /* TODO: read in key_len and value_len */
        deallocator(t, p, sizeof(critbit0_node));
    }
}

void critbit0_clear(critbit0_tree * t)
{
    if (t->root_offset)
        traverse(t, t->root_offset);
    t->root_offset = 0;
}

static int
allprefixed_traverse(critbit0_tree * t, uint64_t top,
                     int (*handle) (const char *, uint32_t, const char *, uint32_t, void *),
                     void *arg)
{
    uint32_t key_len;
    uint32_t value_len;

    if (1 & top) {
        critbit0_node *q = ffa_get_memory(t->ffa, top - 1);
        for (int direction = 0; direction < 2; ++direction)
            switch (allprefixed_traverse(t, q->child[direction], handle, arg)) {
            case 1:
                break;
            case 0:
                return 0;
            default:
                return -1;
            }
        return 1;
    }

    memcpy(&key_len, ffa_get_memory(t->ffa, top), sizeof(uint32_t));
    memcpy(&value_len, ffa_get_memory(t->ffa, top + sizeof(uint32_t)), sizeof(uint32_t));

    return handle((const char *) ffa_get_memory(t->ffa, top + sizeof(uint32_t) + sizeof(uint32_t)),
                  key_len, (const char *) ffa_get_memory(t->ffa, top + sizeof(uint32_t) + sizeof(uint32_t) + key_len), value_len, arg);
}

int
critbit0_allprefixed(critbit0_tree * t, const char *prefix, uint32_t prefix_len,
                     int (*handle) (const char *, uint32_t, const char *, uint32_t, void *),
                     void *arg)
{
    const uint8_t *ubytes = (void *) prefix;
    uint64_t p = t->root_offset;
    uint64_t top = p;

    if (!p)
        return 1;

    while (1 & p) {
        critbit0_node *q = ffa_get_memory(t->ffa, p - 1);
        uint8_t c = 0;
        if (q->byte < prefix_len)
            c = ubytes[q->byte];
        const int direction = (1 + (q->otherbits | c)) >> 8;
        p = q->child[direction];
        if (q->byte < prefix_len)
            top = p;
    }

    for (size_t i = 0; i < prefix_len; ++i) {
        if (((char *)ffa_get_memory(t->ffa, p))[i] != ubytes[i])
            return 1;
    }

    return allprefixed_traverse(t, top, handle, arg);
}
