#define _POSIX_C_SOURCE 200112

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <errno.h>

#include "nutrient.h"
#include "nutrient_util.h"
#include "nutrient_ffa.h"

typedef struct
{
    uint64_t child[2];
    uint32_t byte;
    uint8_t otherbits;
} critbit0_node;

static void read_critbit0_node(critbit0_tree * tree, uint64_t offset,
                               critbit0_node * node)
{
    const char *memory = ffa_get_memory(tree->ffa, offset);

    uint64_unpack(memory, &node->child[0]);
    uint64_unpack(memory + 8, &node->child[1]);
    uint32_unpack(memory + 16, &node->byte);
    node->otherbits = (uint8_t) memory[20];
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
}

static uint64_t add_critbit0_node(critbit0_tree * tree, critbit0_node * node)
{
    /* Allocate 24 bytes for the node storage */
    uint64_t offset = ffa_alloc(tree->ffa, 24);

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

critbit0_tree *critbit0_open(const char *filename)
{
    struct ffa *f;
    critbit0_tree *tree;

    f = ffa_open(filename);
    if (f == NULL) {
        return NULL;
    }

    /* Confirm that the first 8 bytes are our magic number */
    if (ffa_get_size(f) < 24 || memcmp(ffa_get_memory(f, 0), "CrItBiT0", 8))
    {
        return NULL;
    }

    tree = malloc(sizeof(critbit0_tree));
    if (tree == NULL) {
        return NULL;
    }

    /* Unpack the root offset */
    uint64_unpack(ffa_get_memory(f, 8), &tree->root_offset);

    /* Set the ffa */
    tree->ffa = f;

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

static void *_critbit0_find_predecessor(critbit0_tree * t, const char *key, uint32_t key_len)
{
    uint64_t longest_prefix = t->root_offset;
    critbit0_node q = {{ 0 }};

    if (!longest_prefix)
        return NULL;

    while (1 & longest_prefix) {
        read_critbit0_node(t, longest_prefix - 1, &q);

        uint8_t c = 0;
        if (q.byte < key_len)
            c = key[q.byte];

        const int direction = (1 + (q.otherbits | c)) >> 8;
        longest_prefix = q.child[direction];
    }

    return ffa_get_memory(t->ffa, longest_prefix);
}

static void *_critbit0_find(critbit0_tree * t, const char *key,
                            uint32_t key_len)
{
    uint64_t p = t->root_offset;
    uint32_t found_key_len;
    critbit0_node q = {{ 0 }};

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

    uint32_unpack(ffa_get_memory(t->ffa, p), &found_key_len);
    char * found = ffa_get_memory(t->ffa, p);

    if (found_key_len == key_len
        && 0 == memcmp(key, found + 8, key_len)) {
        return found;
    }
    
    return NULL;
}

int critbit0_find(critbit0_tree * t, const char *key, uint32_t key_len,
                  const char **value, uint32_t * value_len)
{
    char *r = _critbit0_find(t, key, key_len);;

    if (!r)
        return -1;

    /* Copy the value length */
    uint32_unpack(r + 4, value_len);

    /* Point the value to the right data */
    *value = (const char *) r + 8 + key_len;

    return 0;
}

int critbit0_find_predecessor(critbit0_tree * t, const char *key, uint32_t key_len,
                                const char **prefix, uint32_t * prefix_len,
                                const char **value, uint32_t * value_len)
{
    char *r = _critbit0_find_predecessor(t, key, key_len);;

    if (!r)
        return -1;

    /* Copy the prefix length */
    uint32_unpack(r, prefix_len);

    /* Copy the value length */
    uint32_unpack(r + 4, value_len);

    /* Point the found to the right data */
    *prefix = (const char *) r + 8;

    /* Point the value to the right data */
    *value = (const char *) r + 8 + *prefix_len;

    return 0;
}

int critbit0_insert(critbit0_tree * t, const char *key, uint32_t key_len,
                    const char *value, uint32_t value_len)
{
    const uint8_t *const ubytes = (void *) key;
    uint64_t p = t->root_offset;
    critbit0_node q;
    critbit0_node newnode;

    uint64_t new_data_offset = ffa_alloc(t->ffa, 8 + key_len + value_len);
    if (new_data_offset == FFA_ERROR)
        return 0;

    /* Copy the key and value length */
    uint32_pack(key_len, ffa_get_memory(t->ffa, new_data_offset));
    uint32_pack(value_len, ffa_get_memory(t->ffa, new_data_offset + 4));

    /* Copy the key */
    memcpy(ffa_get_memory(t->ffa, new_data_offset + 8), key, key_len);

    /* Copy the value */
    memcpy(ffa_get_memory(t->ffa, new_data_offset + 8 + key_len), value, value_len);

    /* If there's no root node, then this key is automatically
     ** the new root node.
     */
    if (!p) {
        /* Create a root node */
        t->root_offset = new_data_offset;

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

    uint32_t newbyte;
    uint32_t newotherbits = 0;
    uint8_t * found_key = ffa_get_memory(t->ffa, p + 8);

    /* Compare the key to the node, until we find a difference */
    for (newbyte = 0; newbyte < key_len; ++newbyte) {
        if (found_key[newbyte] != ubytes[newbyte]) {
            newotherbits = found_key[newbyte] ^ ubytes[newbyte];
            goto different_byte_found;
        }
    }

    /* The key is identical to p, right up to just before "newotherbits" */
    if (newbyte != key_len)
    {
        newotherbits = found_key[newbyte];
        goto different_byte_found;
    }

    /* If we got to here, there's we're inserting a duplicate key, set
    ** new direction to 1 so that new nodes precede old nodes. 
    */
    int newdirection = 1;

    goto node_insertion;

  different_byte_found:

    /* This crazy scheme finds the first (least-significant) bit that
    ** is set and makes it such that it is the only bit set. 
    */
    while (newotherbits & (newotherbits - 1))
        newotherbits &= newotherbits - 1;

    /* Invert the bit pattern */
    newotherbits ^= 255;

    /* What direction from the parent should we insert the key ? */
    uint8_t c = found_key[newbyte];
    newdirection = (1 + (newotherbits | c)) >> 8;

  node_insertion:

    newnode.byte = newbyte;
    newnode.otherbits = newotherbits;
    newnode.child[1 - newdirection] = new_data_offset;

    /* 
     ** We start off with the root node as the insertion point,
     ** and the root node is always 8 bytes from the base. 
     */
    uint64_t offset_of_insertion_point = t->root_offset;
    uint64_t offset_of_pointer_to_insertion_point = 8;

    for (;;) {
        if (!(1 & offset_of_insertion_point))
            break;

        read_critbit0_node(t, offset_of_insertion_point - 1, &q);

        if (q.byte > newbyte)
            break;
        if (q.byte == newbyte && q.otherbits > newotherbits)
            break;
        uint8_t c = 0;
        if (q.byte < key_len)
            c = ubytes[q.byte];
        const int direction = (1 + (q.otherbits | c)) >> 8;

        offset_of_pointer_to_insertion_point = (offset_of_insertion_point - 1) + (direction * 8);
        offset_of_insertion_point = q.child[direction];
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

int critbit0_delete(critbit0_tree * t, const char *key, uint32_t key_len, const char * value, uint32_t value_len)
{
#if 0
    const uint8_t *ubytes = (void *) key;
    uint64_t p = t->root_offset;
    uint64_t offset_of_deletion_point = t->root_offset;
    uint64_t offset_of_pointer_to_deletion_point = 8;
    critbit0_node q = {{ 0 }};
    int direction = 0;
    uint32_t found_key_len;

    if (!t->root_offset)
        return 0;

    while (1 & offset_of_deletion_point) {
        whereq = wherep;

        read_critbit0_node(t, offset_of_deletion_point - 1, &q);

        uint8_t c = 0;
        if (q.byte < key_len)
            c = ubytes[q.byte];

        direction = (1 + (q->otherbits | c)) >> 8;

        offset_of_pointer_to_deletion_point = (offset_of_deletion_point - 1) + (direction * 8);
        offset_of_deletion_point = q.child[direction];
    }

    memcpy(&found_key_len, ffa_get_memory(t->ffa, p), sizeof(uint32_t));

    if (found_key_len != key_len
        || 0 != memcmp(key,
                       ffa_get_memory(t->ffa, p + sizeof(uint32_t) +
                                      sizeof(uint32_t)), key_len))
        return 0;

    // TODO: deallocator(p, sizeof(uint32_t) + sizeof(uint32_t) + key_len + found_value_len);

    if (!whereq) {
        t->root_offset = 0;
        update_root_offset(t);
        return 1;
    }

    *whereq = q.child[1 - direction];
    if (whereq == &t->root_offset) {
        update_root_offset(t);
    }

    // TODO: deallocator(q, sizeof(critbit0_node));
#endif

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

    uint32_unpack(ffa_get_memory(t->ffa, top), &key_len);
    uint32_unpack(ffa_get_memory(t->ffa, top + 4), &value_len);
    const char * key = ffa_get_memory(t->ffa, top + 8);
    const char * value = ffa_get_memory(t->ffa, top + 8 + key_len);
    
    return handle(key, key_len, value, value_len, arg);
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
        const char * found_key = ffa_get_memory(t->ffa, p + 8);
        if (found_key[i] != ubytes[i])
            return 1;
    }

    return allprefixed_traverse(t, top, handle, arg);
}

#if 0

static void print_node(int level, critbit0_tree * t, uint64_t offset)
{
    int i;

    for (i = 0; i < level; i++) {
        printf("    ");
    }

    printf("(%llu) ", offset);

    if (offset & 1) {
        critbit0_node node;

        read_critbit0_node(t, offset - 1, &node);
        
        printf("byte: %lu bits: %u\n", node.byte, (unsigned char) node.otherbits); 

        /* Descend */
        print_node(level + 1, t, node.child[0]);
        print_node(level + 1, t, node.child[1]);
    }
    else {
        uint32_t key_len;
        uint32_t value_len;
        const char * key;
        const char * value;
    
        uint32_unpack(ffa_get_memory(t->ffa, offset), &key_len);
        uint32_unpack(ffa_get_memory(t->ffa, offset + 4), &value_len);

        key = ffa_get_memory(t->ffa, offset + 8);
        value = ffa_get_memory(t->ffa, offset + 8 + key_len);

        printf("data node: klen=%lu vlen=%lu key=%s value=%s\n", key_len, value_len, key, value);
    
        return;
    }
}

void print_tree(critbit0_tree * t)
{
    if (t->root_offset == 0) {
        printf("[ EMPTY TREE ]\n");
    }
    else 
        print_node(0, t, t->root_offset);
    
}

#endif
