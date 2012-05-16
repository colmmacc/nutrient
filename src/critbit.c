#define _POSIX_C_SOURCE 200112

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <errno.h>

#include "critbit.h"
#include "util.h"
#include "ffa.h"

typedef struct
{
    uint64_t child[2];
    uint32_t byte;
    uint8_t otherbits;
} critbit0_node;

static void debug_node(const char * verb, critbit0_node * node, uint64_t offset)
{
//    printf("%s (%llu) child0: %llu child1: %llu byte: %lu bits: %d\n", verb, offset, node->child[0], node->child[1], node->byte, node->otherbits);
}

static void read_critbit0_node(critbit0_tree * tree, uint64_t offset,
                               critbit0_node * node)
{
    const char *memory = ffa_get_memory(tree->ffa, offset);

    uint64_unpack(memory, &node->child[0]);
    uint64_unpack(memory + 8, &node->child[1]);
    uint32_unpack(memory + 16, &node->byte);
    node->otherbits = (uint8_t) memory[20];

    debug_node("read", node, offset);
}

static void update_critbit0_node(critbit0_tree * tree, uint64_t offset,
                                 critbit0_node * node)
{
    char *memory = ffa_get_memory(tree->ffa, offset);

    /* Pack everything in */
    uint64_pack(node->child[0], memory);
    uint64_pack(node->child[1], memory + 8);
    uint32_pack(node->byte, memory + 16);
    memory[20] = (char) node->otherbits;

    debug_node("update", node, offset);
}

static uint64_t add_critbit0_node(critbit0_tree * tree, critbit0_node * node)
{
    /* Allocate 21 bytes for the node storage */
    uint64_t offset = ffa_alloc(tree->ffa,
                                sizeof(uint64_t) + sizeof(uint64_t) +
                                sizeof(uint32_t) + 1);
    if (offset == FFA_ERROR) {
        return FFA_ERROR;
    }

    update_critbit0_node(tree, offset, node);

    return offset;
}

static void update_root_offset(critbit0_tree * tree)
{
    uint64_pack(tree->root_offset, ffa_get_memory(tree->ffa, 8));
}

critbit0_tree *critbit0_create(const char *filename)
{
    struct ffa *f;
    uint64_t r;
    critbit0_tree *tree;

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
    if (r == FFA_ERROR) {
        return NULL;
    }

    /* Copy our magic number in */
    memcpy(ffa_get_memory(f, r), "CrItBiT0", 8);

    /* Update the root offset */
    update_root_offset(tree);

    /* Sync the file */
    ffa_sync(f);

    return tree;
}

int critbit0_sync(critbit0_tree * t)
{
    return ffa_sync(t->ffa);
}

int critbit0_close(critbit0_tree * t)
{
    return ffa_close(t->ffa);
}

static void *_critbit0_find(critbit0_tree * t, const char *key,
                            uint32_t key_len)
{
    uint64_t p = t->root_offset;
    uint32_t found_key_len;
    uint32_t found_value_len;
    critbit0_node q;

    if (!p)
        return NULL;

    while (1 & p) {
        read_critbit0_node(t, p - 1, &q);
        uint8_t c = 0;
        if (q.byte < key_len)
            c = key[q.byte];
        const int direction = (1 + (q.otherbits | c)) >> 8;

        p = q.child[direction];
    }

    memcpy(&found_key_len, ffa_get_memory(t->ffa, p), sizeof(uint32_t));
    memcpy(&found_value_len, ffa_get_memory(t->ffa, p + sizeof(uint32_t)),
           sizeof(uint32_t));

    if (found_key_len == key_len
        && 0 == memcmp(key,
                       ffa_get_memory(t->ffa,
                                      p + sizeof(uint32_t) +
                                      sizeof(uint32_t)), key_len)) {
        return ffa_get_memory(t->ffa, p);
    }

    return NULL;
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
    critbit0_node q;

    /* If there's no root node, then this key is automatically
     ** the new root node.
     */
    if (!p) {
        uint64_t x = ffa_alloc(t->ffa,
                               sizeof(key_len) + key_len + sizeof(value_len) +
                               value_len);
        if (x == FFA_ERROR)
            return 0;

        /* Copy the key and value length */
        memcpy(ffa_get_memory(t->ffa, x), &key_len, sizeof(uint32_t));
        memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t)), &value_len,
               sizeof(uint32_t));

        /* Copy the key */
        memcpy(ffa_get_memory
               (t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t)), key,
               key_len);

        /* Copy the value */
        memcpy(ffa_get_memory
               (t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t) + key_len),
               value, value_len);

        /* Create a root node */
        t->root_offset = x;

        /* Update the saved copy of the root offset */
        update_root_offset(t);

        return 2;
    }

    /* Keep decending into children, as long as the LSB is "1" */
    while (1 & p) {
        read_critbit0_node(t, p - 1, &q);
        uint8_t c = 0;
        if (q.byte < key_len)
            c = ubytes[q.byte];

        /* Pick the direction */
        const int direction = (1 + (q.otherbits | c)) >> 8;
        p = q.child[direction];
    }

    /* At this point, p is the offset of the closest match node to our key,
     ** possibly an exact match - if not, then a viable parent node 
     */

    uint32_t newbyte;
    uint32_t newotherbits;

    /* Compare the key to the node, until we find a difference */
    for (newbyte = 0; newbyte < key_len; ++newbyte) {
        if (((char *)
             ffa_get_memory(t->ffa,
                            p + sizeof(uint32_t) +
                            sizeof(uint32_t)))[newbyte] != ubytes[newbyte]) {
            newotherbits = ((char *)
                            ffa_get_memory(t->ffa,
                                           p + sizeof(uint32_t) +
                                           sizeof(uint32_t)))[newbyte] ^
                ubytes[newbyte];
            goto different_byte_found;
        }
    }

    /* The key is identical to p, right up to just before "newotherbits" */
    if (((char *)
         ffa_get_memory(t->ffa,
                        p + sizeof(uint32_t) + sizeof(uint32_t)))[newbyte] !=
        0) {
        newotherbits = ((char *)
                        ffa_get_memory(t->ffa,
                                       p + sizeof(uint32_t) +
                                       sizeof(uint32_t)))[newbyte];
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
    uint8_t c = ((char *)
                 ffa_get_memory(t->ffa,
                                p + sizeof(uint32_t) +
                                sizeof(uint32_t)))[newbyte];
    int newdirection = (1 + (newotherbits | c)) >> 8;

    critbit0_node newnode;

    /* Allocate space for its data */
    uint64_t x = ffa_alloc(t->ffa,
                           sizeof(uint32_t) + sizeof(uint32_t) + key_len +
                           value_len);
    if (x == FFA_ERROR) {
        return 0;
    }

    memcpy(ffa_get_memory(t->ffa, x), &key_len, sizeof(uint32_t));
    memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t)), &value_len,
           sizeof(uint32_t));
    memcpy(ffa_get_memory(t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t)),
           key, key_len);
    memcpy(ffa_get_memory
           (t->ffa, x + sizeof(uint32_t) + sizeof(uint32_t) + key_len), value,
           value_len);

    newnode.byte = newbyte;
    newnode.otherbits = newotherbits;
    newnode.child[1 - newdirection] = x;

    /* 
     ** We start off with the root node as the insertion point,
     ** and the root node is always 8 bytes from the base. We
     ** use 9, because pointers to nodes always have the LSB set
     ** to 1.
     */
    uint64_t offset_of_insertion_point = t->root_offset;
    uint64_t offset_of_pointer_to_insertion_point = 8;

    for (;;) {
        if (!(1 & offset_of_insertion_point))
            break;

        read_critbit0_node(t, p - 1, &q);

        if (q.byte > newbyte)
            break;
        if (q.byte == newbyte && q.otherbits > newotherbits)
            break;
        uint8_t c = 0;
        if (q.byte < key_len)
            c = ubytes[q.byte];
        const int direction = (1 + (q.otherbits | c)) >> 8;

        offset_of_insertion_point = q.child[direction];
        offset_of_pointer_to_insertion_point = p - 1 + (direction * 8);
    }

    /* The new node will be the parent of the insertion point */
    newnode.child[newdirection] = offset_of_insertion_point;

    /* Add the new node */
    uint64_t newnode_offset = add_critbit0_node(t, &newnode);

    /* Update the memory that originally pointed at the insertion point, to point at the
     ** new node. This may even be the root node.
     */
    uint64_pack(1 + newnode_offset,
                ffa_get_memory(t->ffa, offset_of_pointer_to_insertion_point));

    /* Update the in-memory root offset too */
    if (offset_of_pointer_to_insertion_point == 8) {
        t->root_offset = 1 + newnode_offset;
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
    memcpy(&found_value_len, ffa_get_memory(t->ffa, p + sizeof(uint32_t)),
           sizeof(uint32_t));

    if (found_key_len != key_len
        || 0 != memcmp(key,
                       ffa_get_memory(t->ffa, p + sizeof(uint32_t) +
                                      sizeof(uint32_t)), key_len))
        return 0;

    //deallocator(p, sizeof(uint32_t) + sizeof(uint32_t) + key_len + found_value_len);

    if (!whereq) {
        t->root_offset = 0;
        update_root_offset(t);
        return 1;
    }

    *whereq = q->child[1 - direction];
    if (whereq == &t->root_offset) {
        update_root_offset(t);
    }

    //deallocator(q, sizeof(critbit0_node));

    return 1;
}

void critbit0_clear(critbit0_tree * t)
{
    t->root_offset = 0;
    update_root_offset(t);

    ffa_truncate(t->ffa, 8 + sizeof(uint64_t));
}

static int
allprefixed_traverse(critbit0_tree * t, uint64_t top,
                     int (*handle) (const char *, uint32_t, const char *,
                                    uint32_t, void *), void *arg)
{
    uint32_t key_len;
    uint32_t value_len;
    critbit0_node q;

    if (1 & top) {
        read_critbit0_node(t, top - 1, &q);
        for (int direction = 0; direction < 2; ++direction)
            switch (allprefixed_traverse(t, q.child[direction], handle, arg)) {
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
    memcpy(&value_len, ffa_get_memory(t->ffa, top + sizeof(uint32_t)),
           sizeof(uint32_t));

    return handle((const char *)
                  ffa_get_memory(t->ffa,
                                 top + sizeof(uint32_t) + sizeof(uint32_t)),
                  key_len, (const char *) ffa_get_memory(t->ffa,
                                                         top +
                                                         sizeof(uint32_t) +
                                                         sizeof(uint32_t) +
                                                         key_len), value_len,
                  arg);
}

int
critbit0_allprefixed(critbit0_tree * t, const char *prefix,
                     uint32_t prefix_len, int (*handle) (const char *,
                                                         uint32_t,
                                                         const char *,
                                                         uint32_t, void *),
                     void *arg)
{
    const uint8_t *ubytes = (void *) prefix;
    uint64_t p = t->root_offset;
    uint64_t top = p;
    critbit0_node q;

    if (!p)
        return 1;

    while (1 & p) {
        read_critbit0_node(t, p - 1, &q);
        uint8_t c = 0;
        if (q.byte < prefix_len)
            c = ubytes[q.byte];
        const int direction = (1 + (q.otherbits | c)) >> 8;
        p = q.child[direction];
        if (q.byte < prefix_len)
            top = p;
    }

    for (size_t i = 0; i < prefix_len; ++i) {
        if (((char *) ffa_get_memory(t->ffa, p))[i] != ubytes[i])
            return 1;
    }

    return allprefixed_traverse(t, top, handle, arg);
}
