#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "nutrient.h"
#include "critbit.h"
#include "ffa.h"

int allprefixed_cb(const char *key, uint32_t key_len, const char *value,
                   uint32_t value_len, void *arg)
{
    printf("key_len=%u value_len=%u key=%s value=%s\n", key_len, value_len,
           key, value);
    return 1;
}

int cidr_cb(const char *key, uint32_t key_len, const char *value,
                   uint32_t value_len, void *arg)
{
    int i;
    
    for (i = 0; i < key_len; i++)
    {
        if (i != 0) {
            printf(".");
        }
        printf("%d", (unsigned char) key[i]);
    }

    for (; i < 4; i++) {
        printf(".0");
    }

    printf(" %d \n", key_len);

    return 1;
}
int test_critbit1()
{
    critbit0_tree *tree;
    const char *value;
    const char *prefix;
    uint32_t value_len;
    uint32_t prefix_len;

    unlink("2");
    tree = critbit0_create("2");

    critbit0_insert(tree, "\001", 1, "1", 2);
    critbit0_insert(tree, "\002", 1, "2", 2);
    critbit0_insert(tree, "\003", 1, "3", 2);
    critbit0_insert(tree, "\004", 1, "4", 2);

    print_tree(tree);

    critbit0_sync(tree);
    critbit0_close(tree);

    return 0;
}

int test_critbit0()
{
    critbit0_tree *tree;
    const char *value;
    const char *prefix;
    uint32_t value_len;
    uint32_t prefix_len;

    unlink("trie");
    tree = critbit0_create("trie");

    critbit0_insert(tree, "colm", 5, "657", 4);
    critbit0_insert(tree, "columnar", 9, "822", 4);
    critbit0_insert(tree, "veronica", 9, "123", 4);
    critbit0_insert(tree, "coln", 5, "321", 4);
    critbit0_insert(tree, "veronica", 9, "456", 4);
    critbit0_insert(tree, "colm", 5, "456", 4);
    critbit0_insert(tree, "colm", 5, "333", 4);
    critbit0_insert(tree, "colm", 5, "777", 4);
    critbit0_insert(tree, "colman", 7, "777", 4);

    printf("All prefixed col:\n");
    critbit0_allprefixed(tree, "col", 3, allprefixed_cb, NULL);

    printf("All prefixed:\n");
    critbit0_allprefixed(tree, "", 0, allprefixed_cb, NULL);

    printf("colmus: %d ", critbit0_find_longest_prefix(tree, "colm\0us", 8, &prefix, &prefix_len, &value, &value_len));
    printf("prefix: %s prefix_len: %lu value_len: %lu value: %s\n", prefix, prefix_len, value_len, value);
 
    //critbit0_delete(tree, "colm", 4);

    printf("\n");
    //critbit0_allprefixed(tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    printf("veronica: %d ",
           critbit0_find(tree, "veronica", 9, &value, &value_len));
    printf("value_len: %u value: %s\n", value_len, value);
    printf("bat:  %d\n", critbit0_find(tree, "bat", 4, &value, &value_len));

    //critbit0_delete(&tree, "veronica", 9);

    printf("veronica: %d\n",
           critbit0_find(tree, "veronica", 9, &value, &value_len));
    printf("bat:  %d\n", critbit0_find(tree, "bat", 4, &value, &value_len));

    printf("\n");
    //critbit0_allprefixed(tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    print_tree(tree);

    critbit0_sync(tree);
    critbit0_close(tree);

    return 0;
}

int test_ffa()
{
    struct ffa *handle;
    char *offset;

    handle = ffa_create("stupid");

    offset = ffa_alloc(handle, 10);
    printf("offset: %p\n", offset);

    strcpy(offset, "colm");

    offset = ffa_alloc(handle, 20);
    printf("offset: %p\n", offset);
    strcpy(offset, "veronica");

    offset = ffa_alloc(handle, 30);
    printf("offset: %p\n", offset);
    strcpy(offset, "foobar");

    ffa_sync(handle);
    ffa_close(handle);

    unlink("stupid");

    return 0;
}

int test_cidr()
{
    critbit0_tree *tree;
    unsigned char ip1[4] = { 10, 0, 0, 0 };
    unsigned char ip2[4] = { 10, 0, 3, 0 };
    unsigned char ip3[4] = { 10, 0, 3, 128 };
    unsigned char ip4[4] = { 10, 7, 6, 8 };
    unsigned char ip5[4] = { 10, 0, 3, 126 };
    unsigned char ip6[4] = { 10, 0, 3, 129 };
    const char * prefix;
    const char * value;
    uint32_t prefix_len;
    uint32_t value_len;

    unlink("cidr");
    tree = critbit0_create("cidr");

    critbit0_insert(tree, (const char *) ip1, 1, "123", 4);
    critbit0_insert(tree, (const char *) ip2, 3, "456", 4);
    critbit0_insert(tree, (const char *) ip3, 4, "789", 4);

    printf("All CIDRs:\n");
    critbit0_allprefixed(tree, "", 0, cidr_cb, NULL);

    printf("Longest match:\n");

    printf("10.6.7.8: %d\n", critbit0_find_longest_prefix(tree, ip4, 4, &prefix, &prefix_len, &value, &value_len));

//    printf("prefix: %u.%u.%u.%u prefix_len: %lu value_len: %lu value: %s\n", prefix[0], prefix[1], prefix[2], (unsigned char) prefix[3], prefix_len, value_len, value);
    print_tree(tree);

    critbit0_sync(tree);
    critbit0_close(tree);

    return 0;
}


int main(int argc, char **argv)
{
    test_cidr();

    test_critbit0();
    printf("\n");
    test_critbit1();

    //test_ffa();

    return 0;
}
