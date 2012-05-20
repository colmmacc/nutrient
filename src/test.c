#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#include "nutrient.h"
#include "nutrient_ffa.h"
#include "nutrient_util.h"

int allprefixed_cb(const char *key, uint32_t key_len, const char *value,
                   uint32_t value_len, void *arg)
{
    printf("key_len=%u value_len=%u key=", key_len, value_len);
    fwrite(key, key_len, 1, stdout);
    printf(" value=");
    fwrite(value, value_len, 1, stdout);
    printf("\n");

    return 1;
}

int cidr_cb(const char *key, uint32_t key_len, const char *value,
                   uint32_t value_len, void *arg)
{
    printf("key=");
    fwrite(key, key_len, 1, stdout);
    printf(" value=");
    fwrite(value, value_len, 1, stdout);
    printf("\n");



    return 1;
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

    printf("colmus: %d ", critbit0_find_predecessor(tree, "colm\0us", 8, &prefix, &prefix_len, &value, &value_len));
    printf("prefix: %s prefix_len: %" PRIu32 " value_len: %" PRIu32 " value: %s\n", prefix, prefix_len, value_len, value);
 
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

    // print_tree(tree);

    critbit0_sync(tree);
    critbit0_close(tree);

    return 0;
}

int test_cidr()
{
    critbit0_tree *tree;
    unsigned char ip1[4] = { 10, 0, 0, 0 };
    unsigned char ip2[4] = { 10, 0, 3, 0 };
    unsigned char ip3[4] = { 10, 0, 3, 128 };
    unsigned char ip4[4] = { 129, 7, 6, 8 };

    unsigned char ip5[4] = { 10, 0, 3, 1 };
    unsigned char ip6[4] = { 10, 0, 3, 137 };
    unsigned char ip7[4] = { 10, 8, 0, 0 };
    unsigned char ip8[4] = { 11, 0, 0, 0 };

    char ip1_str[33];
    char ip2_str[33];
    char ip3_str[33];
    char ip4_str[33];
    char ip5_str[33];
    char ip6_str[33];
    char ip7_str[33];
    char ip8_str[33];

    const char * prefix;
    const char * value;
    uint32_t prefix_len;
    uint32_t value_len;
    

    ipv4_cidr_pack(ip1, 8, ip1_str);
    ipv4_cidr_pack(ip2, 24, ip2_str);
    ipv4_cidr_pack(ip3, 25, ip3_str);
    ipv4_cidr_pack(ip4, 32, ip4_str);
    ipv4_cidr_pack(ip5, 32, ip5_str);
    ipv4_cidr_pack(ip6, 32, ip6_str);
    ipv4_cidr_pack(ip7, 32, ip7_str);
    ipv4_cidr_pack(ip8, 32, ip8_str);

    ipv4_cidr_unpack(ip1_str, 8, ip1);
    ipv4_cidr_unpack(ip2_str, 24, ip2);
    ipv4_cidr_unpack(ip3_str, 25, ip3);
    ipv4_cidr_unpack(ip4_str, 32, ip4);

    unlink("cidr");
    tree = critbit0_create("cidr");

    critbit0_insert(tree, ip1_str, 8,  "10.0.0.0/8", 11);
    critbit0_insert(tree, ip2_str, 24, "10.0.3.0/24", 12);
    critbit0_insert(tree, ip3_str, 25, "10.0.3.128/25", 14);
    critbit0_insert(tree, ip4_str, 32, "129.7.6.8/32", 13);

    printf("All CIDRs:\n");
    critbit0_allprefixed(tree, "", 0, cidr_cb, NULL);

    printf("Longest match:\n");

    printf("129.7.6.8 (%s): %d ", ip4_str, critbit0_find_predecessor(tree, ip4_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("10.0.3.1 (%s): %d ", ip5_str, critbit0_find_predecessor(tree, ip5_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("10.0.3.137 (%s): %d ", ip6_str, critbit0_find_predecessor(tree, ip6_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("10.8.0.0 (%s): %d ", ip7_str, critbit0_find_predecessor(tree, ip7_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("11.0.0.0 (%s): %d ", ip6_str, critbit0_find_predecessor(tree, ip8_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    print_tree(tree);

    printf("\n");

    critbit0_sync(tree);
    critbit0_close(tree);

    return 0;
}


int main(int argc, char **argv)
{
    test_cidr();

    test_critbit0();
    printf("\n");

    return 0;
}
