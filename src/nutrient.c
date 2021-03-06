#define _POSIX_C_SOURCE 200112

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <errno.h>

#include "nutrient.h"
#include "nutrient_util.h"
#include "nutrient_ffa.h"

struct nutrient_tree {
    uint64_t root_offset;
    struct ffa *ffa;
};

typedef struct {
    uint64_t child[2];
    uint32_t byte;
    uint8_t otherbits;
}      nutrient_node;

static void read_nutrient_node(struct nutrient_tree * tree, uint64_t offset,
			            nutrient_node * node)
{
    const uint8_t *memory = ffa_get_memory(tree->ffa, offset);

    uint64_unpack(memory, &node->child[0]);
    uint64_unpack(memory + 8, &node->child[1]);
    uint32_unpack(memory + 16, &node->byte);
    node->otherbits = (uint8_t) memory[20];
}

#ifdef DEBUG
static void print_node(int level, struct nutrient_tree * t, uint64_t offset)
{
    int i;

    for (i = 0; i < level; i++) {
	printf("    ");
    }

    printf("(%llu) ", offset);

    if (offset & 1) {
	nutrient_node node;

	read_nutrient_node(t, offset - 1, &node);

	printf("byte: %u bits: %u\n", node.byte, (unsigned char) node.otherbits);

	/* Descend */
	print_node(level + 1, t, node.child[0]);
	print_node(level + 1, t, node.child[1]);
    }
    else {
	uint32_t key_len;
	uint32_t value_len;
	const uint8_t *key;
	const uint8_t *value;

	uint32_unpack(ffa_get_memory(t->ffa, offset), &key_len);
	uint32_unpack(ffa_get_memory(t->ffa, offset + 4), &value_len);

	key = ffa_get_memory(t->ffa, offset + 8);
	value = ffa_get_memory(t->ffa, offset + 8 + key_len);

	printf("data node: klen=%u vlen=%u key=", key_len, value_len);
	fwrite(key, key_len, 1, stdout);
	printf(" value=");
	fwrite(value, value_len, 1, stdout);
	printf("\n");


	return;
    }
}

void print_tree(struct nutrient_tree * t)
{
    if (t->root_offset == 0) {
	printf("[ EMPTY TREE ]\n");
    }
    else
	print_node(0, t, t->root_offset);

}
#endif

static void update_nutrient_node(struct nutrient_tree * tree, uint64_t offset,
				      nutrient_node * node)
{
    uint8_t *memory = ffa_get_memory(tree->ffa, offset);

    /* Pack everything in */
    uint64_pack(node->child[0], memory);
    uint64_pack(node->child[1], memory + 8);
    uint32_pack(node->byte, memory + 16);
    memory[20] = (char) node->otherbits;
}

static uint64_t add_nutrient_node(struct nutrient_tree * tree, nutrient_node * node)
{
    /* Allocate 24 bytes for the node storage */
    uint64_t offset = ffa_alloc(tree->ffa, 24);

    if (offset == FFA_ERROR) {
	return FFA_ERROR;
    }

    update_nutrient_node(tree, offset, node);

    return offset;
}

static void update_root_offset(struct nutrient_tree * tree)
{
    uint64_pack(tree->root_offset, ffa_get_memory(tree->ffa, 8));
}

struct nutrient_tree *nutrient_create(const char *filename)
{
    struct ffa *f;
    uint64_t r;
    struct nutrient_tree *tree;

    f = ffa_create(filename);
    if (f == NULL) {
	return NULL;
    }

    tree = malloc(sizeof(struct nutrient_tree));
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

struct nutrient_tree *nutrient_open(const char *filename)
{
    struct ffa *f;
    struct nutrient_tree *tree;

    f = ffa_open(filename);
    if (f == NULL) {
	return NULL;
    }

    /* Confirm that the first 8 bytes are our magic number */
    if (ffa_get_size(f) < 24 || memcmp(ffa_get_memory(f, 0), "CrItBiT0", 8)) {
	return NULL;
    }

    tree = malloc(sizeof(struct nutrient_tree));
    if (tree == NULL) {
	return NULL;
    }

    /* Unpack the root offset */
    uint64_unpack(ffa_get_memory(f, 8), &tree->root_offset);

    /* Set the ffa */
    tree->ffa = f;

    return tree;
}

int nutrient_sync(struct nutrient_tree * t)
{
    return ffa_sync(t->ffa);
}

int nutrient_close(struct nutrient_tree * t)
{
    return ffa_close(t->ffa);
}

static void *_nutrient_find_predecessor(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len)
{
    uint64_t predecessor = t->root_offset;
    nutrient_node q = {{0}};

    if (!predecessor)
	return NULL;

    while (1 & predecessor) {
	read_nutrient_node(t, predecessor - 1, &q);

	uint8_t c = 0;
	if (q.byte < key_len)
	    c = key[q.byte];

	const int direction = (1 + (q.otherbits | c)) >> 8;
	predecessor = q.child[direction];
    }

    return ffa_get_memory(t->ffa, predecessor);
}

static void *_nutrient_find_longest_prefix(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len)
{
    uint8_t *found = _nutrient_find_predecessor(t, key, key_len);

    if (found == NULL) {
	return NULL;
    }

    uint32_t newbyte;
    uint32_t newotherbits = 0;
    uint8_t *found_key = found + 8;
    uint32_t found_key_len;
    uint64_t longest_prefix_node = t->root_offset;
    uint64_t longest_data_prefix = 0;
    nutrient_node q;

    /* See if the predeccessor is our longest prefix */
    uint32_unpack((uint8_t *) found, &found_key_len);

    /* Compare the key to the node, until we find a difference */
    for (newbyte = 0; newbyte < found_key_len; ++newbyte) {
	if (found_key[newbyte] != key[newbyte]) {
	    newotherbits = found_key[newbyte] ^ key[newbyte];
	    goto different_byte_found;
	}
    }

    if (newbyte == key_len) {
	return found;
    }

    newotherbits = key[newbyte];

different_byte_found:

    for (;;) {
	if (!(1 & longest_prefix_node))
	    break;

	read_nutrient_node(t, longest_prefix_node - 1, &q);

	uint8_t c = 0;
	if (q.byte < key_len)
	    c = key[q.byte];
	const int direction = (1 + (q.otherbits | c)) >> 8;

	longest_prefix_node = q.child[direction];

	if (!(1 & q.child[1 - direction])) {
	    uint8_t *candidate_prefix = ffa_get_memory(t->ffa, q.child[1 - direction]);
	    uint32_t candidate_prefix_len;

	    uint32_unpack((uint8_t *) candidate_prefix, &candidate_prefix_len);

	    if (candidate_prefix_len < key_len && !memcmp(candidate_prefix + 8, key, candidate_prefix_len)) {
		longest_data_prefix = q.child[1 - direction];
	    }
	}

	if (!(1 & q.child[direction])) {
	    uint8_t *candidate_prefix = ffa_get_memory(t->ffa, q.child[direction]);
	    uint32_t candidate_prefix_len;

	    uint32_unpack((uint8_t *) candidate_prefix, &candidate_prefix_len);

	    if (candidate_prefix_len < key_len && !memcmp(candidate_prefix + 8, key, candidate_prefix_len)) {
		longest_data_prefix = q.child[direction];
	    }
	}

	if (q.byte > newbyte)
	    break;

	if (q.byte == newbyte && q.otherbits > newotherbits)
	    break;
    }


    if (longest_data_prefix != 0) {
	return ffa_get_memory(t->ffa, longest_data_prefix);
    }
    else {
	return NULL;
    }
}

static void *_nutrient_find(struct nutrient_tree * t, const uint8_t * key,
			         uint32_t key_len)
{
    uint64_t p = t->root_offset;
    uint32_t found_key_len;
    nutrient_node q = {{0}};

    if (!p)
	return NULL;

    while (1 & p) {
	read_nutrient_node(t, p - 1, &q);
	uint8_t c = 0;
	if (q.byte < key_len)
	    c = key[q.byte];
	const int direction = (1 + (q.otherbits | c)) >> 8;

	p = q.child[direction];
    }

    uint32_unpack(ffa_get_memory(t->ffa, p), &found_key_len);
    uint8_t *found = ffa_get_memory(t->ffa, p);

    if (found_key_len == key_len
	&& 0 == memcmp(key, found + 8, key_len)) {
	return found;
    }

    return NULL;
}

int nutrient_find(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
		      const uint8_t ** value, uint32_t * value_len)
{
    uint8_t *r = _nutrient_find(t, key, key_len);;

    if (!r)
	return -1;

    /* Copy the value length */
    uint32_unpack(r + 4, value_len);

    /* Point the value to the right data */
    *value = (const uint8_t *) r + 8 + key_len;

    return 0;
}

int nutrient_find_longest_prefix(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
			     const uint8_t ** prefix, uint32_t * prefix_len,
			       const uint8_t ** value, uint32_t * value_len)
{
    uint8_t *r = _nutrient_find_longest_prefix(t, key, key_len);;

    if (!r)
	return -1;

    /* Copy the prefix length */
    uint32_unpack(r, prefix_len);

    /* Copy the value length */
    uint32_unpack(r + 4, value_len);

    /* Point the found to the right data */
    *prefix = (const uint8_t *) r + 8;

    /* Point the value to the right data */
    *value = (const uint8_t *) r + 8 + *prefix_len;

    return 0;
}

int nutrient_find_predecessor(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
			     const uint8_t ** prefix, uint32_t * prefix_len,
			       const uint8_t ** value, uint32_t * value_len)
{
    uint8_t *r = _nutrient_find_predecessor(t, key, key_len);;

    if (!r)
	return -1;

    /* Copy the prefix length */
    uint32_unpack(r, prefix_len);

    /* Copy the value length */
    uint32_unpack(r + 4, value_len);

    /* Point the found to the right data */
    *prefix = (const uint8_t *) r + 8;

    /* Point the value to the right data */
    *value = (const uint8_t *) r + 8 + *prefix_len;

    return 0;
}

int nutrient_insert(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len,
		        const uint8_t * value, uint32_t value_len)
{
    const uint8_t *const ubytes = (void *) key;
    uint64_t p = t->root_offset;
    nutrient_node q;
    nutrient_node newnode;

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

    /*
     * If there's no root node, then this key is automatically * the new root
     * node.
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
	read_nutrient_node(t, p - 1, &q);
	uint8_t c = 0;
	if (q.byte < key_len)
	    c = ubytes[q.byte];

	/* Pick the direction */
	const int direction = (1 + (q.otherbits | c)) >> 8;
	p = q.child[direction];
    }

    uint32_t newbyte;
    uint32_t newotherbits = 0;
    uint8_t *found_key = ffa_get_memory(t->ffa, p + 8);

    /* Compare the key to the node, until we find a difference */
    for (newbyte = 0; newbyte < key_len; ++newbyte) {
	if (found_key[newbyte] != ubytes[newbyte]) {
	    newotherbits = found_key[newbyte] ^ ubytes[newbyte];
	    goto different_byte_found;
	}
    }

    /* The key is identical to p, right up to just before "newotherbits" */
    if (newbyte != key_len) {
	newotherbits = found_key[newbyte];
	goto different_byte_found;
    }

    /*
     * If we got to here, there's we're inserting a duplicate key, set * new
     * direction to 1 so that new nodes precede old nodes.
     */
    int newdirection = 1;

    goto node_insertion;

different_byte_found:

    /*
     * This crazy scheme finds the first (least-significant) bit that * is
     * set and makes it such that it is the only bit set.
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
     * * We start off with the root node as the insertion point, * and the
     * root node is always 8 bytes from the base.
     */
    uint64_t offset_of_insertion_point = t->root_offset;
    uint64_t offset_of_pointer_to_insertion_point = 8;

    for (;;) {
	if (!(1 & offset_of_insertion_point))
	    break;

	read_nutrient_node(t, offset_of_insertion_point - 1, &q);

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
    uint64_t newnode_offset = add_nutrient_node(t, &newnode);

    /*
     * Update the memory that originally pointed at the insertion point, to
     * point at the * new node. This may even be the root node.
     */
    uint64_pack(1 + newnode_offset,
	      ffa_get_memory(t->ffa, offset_of_pointer_to_insertion_point));

    /* Update the in-memory root offset too */
    if (offset_of_pointer_to_insertion_point == 8) {
	t->root_offset = 1 + newnode_offset;
    }

    return 2;
}

int nutrient_delete(struct nutrient_tree * t, const uint8_t * key, uint32_t key_len, const uint8_t * value, uint32_t value_len)
{
#if 0
    const uint8_t *ubytes = (void *) key;
    uint64_t p = t->root_offset;
    uint64_t offset_of_deletion_point = t->root_offset;
    uint64_t offset_of_pointer_to_deletion_point = 8;
    nutrient_node q = {{0}};
    int direction = 0;
    uint32_t found_key_len;

    if (!t->root_offset)
	return 0;

    while (1 & offset_of_deletion_point) {
	whereq = wherep;

	read_nutrient_node(t, offset_of_deletion_point - 1, &q);

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

//TODO:deallocator(p, sizeof(uint32_t) + sizeof(uint32_t) + key_len + found_value_len);

    if (!whereq) {
	t->root_offset = 0;
	update_root_offset(t);
	return 1;
    }

    *whereq = q.child[1 - direction];
    if (whereq == &t->root_offset) {
	update_root_offset(t);
    }

//TODO:deallocator(q, sizeof(nutrient_node));
#endif

    return 1;
}

void nutrient_clear(struct nutrient_tree * t)
{
    t->root_offset = 0;
    update_root_offset(t);

    ffa_truncate(t->ffa, 8 + sizeof(uint64_t));
}

static int allprefixed_traverse(struct nutrient_tree * t, uint64_t top,
	          int (*handle) (const uint8_t *, uint32_t, const uint8_t *,
				     uint32_t, void *), void *arg)
{
    uint32_t key_len;
    uint32_t value_len;
    nutrient_node q;

    if (1 & top) {
	read_nutrient_node(t, top - 1, &q);
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
    const uint8_t *key = ffa_get_memory(t->ffa, top + 8);
    const uint8_t *value = ffa_get_memory(t->ffa, top + 8 + key_len);

    return handle(key, key_len, value, value_len, arg);
}

int nutrient_allprefixed(struct nutrient_tree * t, const uint8_t * prefix,
		        uint32_t prefix_len, int (*handle) (const uint8_t *,
							        uint32_t,
							    const uint8_t *,
						          uint32_t, void *),
			     void *arg)
{
    const uint8_t *ubytes = (void *) prefix;
    uint64_t p = t->root_offset;
    uint64_t top = p;
    nutrient_node q;

    if (!p)
	return 1;

    while (1 & p) {
	read_nutrient_node(t, p - 1, &q);
	uint8_t c = 0;
	if (q.byte < prefix_len)
	    c = ubytes[q.byte];
	const int direction = (1 + (q.otherbits | c)) >> 8;
	p = q.child[direction];
	if (q.byte < prefix_len)
	    top = p;
    }

    for (size_t i = 0; i < prefix_len; ++i) {
	const uint8_t *found_key = ffa_get_memory(t->ffa, p + 8);
	if (found_key[i] != ubytes[i])
	    return 1;
    }

    return allprefixed_traverse(t, top, handle, arg);
}
