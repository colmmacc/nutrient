#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#include "nutrient.h"
#include "nutrient_ffa.h"
#include "nutrient_util.h"

int allprefixed_cb(const uint8_t *key, uint32_t key_len, const uint8_t *value,
                   uint32_t value_len, void *arg)
{
    printf("key_len=%u value_len=%u key=", key_len, value_len);
    fwrite(key, key_len, 1, stdout);
    printf(" value=");
    fwrite(value, value_len, 1, stdout);
    printf("\n");

    return 1;
}

int cidr_cb(const uint8_t *key, uint32_t key_len, const uint8_t *value,
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
    struct nutrient_tree *tree;
    const uint8_t *value;
    const uint8_t *prefix;
    uint32_t value_len;
    uint32_t prefix_len;

    unlink("trie");
    tree = nutrient_create("trie");

    nutrient_insert(tree, (uint8_t *) "colm", 5, (uint8_t *) "657", 4);
    nutrient_insert(tree, (uint8_t *) "columnar", 9, (uint8_t *) "822", 4);
    nutrient_insert(tree, (uint8_t *) "veronica", 9, (uint8_t *) "123", 4);
    nutrient_insert(tree, (uint8_t *) "coln", 5, (uint8_t *) "321", 4);
    nutrient_insert(tree, (uint8_t *) "veronica", 9, (uint8_t *) "456", 4);
    nutrient_insert(tree, (uint8_t *) "colm", 5, (uint8_t *) "456", 4);
    nutrient_insert(tree, (uint8_t *) "colm", 5, (uint8_t *) "333", 4);
    nutrient_insert(tree, (uint8_t *) "colm", 5, (uint8_t *) "777", 4);
    nutrient_insert(tree, (uint8_t *) "colman", 7, (uint8_t *) "777", 4);

    printf("All prefixed col:\n");
    nutrient_allprefixed(tree, (uint8_t *) "col", 3, allprefixed_cb, NULL);

    printf("All prefixed:\n");
    nutrient_allprefixed(tree, (uint8_t *) "", 0, allprefixed_cb, NULL);

    printf("colmus: %d ", nutrient_find_predecessor(tree, (uint8_t *) "colm\0us", 8, &prefix, &prefix_len, &value, &value_len));
    printf("prefix: %s prefix_len: %" PRIu32 " value_len: %" PRIu32 " value: %s\n", prefix, prefix_len, value_len, value);
 
    //nutrient_delete(tree, "colm", 4);

    printf("\n");
    //nutrient_allprefixed(tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    printf("veronica: %d ",
           nutrient_find(tree, (uint8_t *) "veronica", 9, &value, &value_len));
    printf("value_len: %u value: %s\n", value_len, value);
    printf("bat:  %d\n", nutrient_find(tree, (uint8_t *) "bat", 4, &value, &value_len));

    //nutrient_delete(&tree, "veronica", 9);

    printf("veronica: %d\n",
           nutrient_find(tree, (uint8_t *) "veronica", 9, &value, &value_len));
    printf("bat:  %d\n", nutrient_find(tree, (uint8_t *) "bat", 4, &value, &value_len));

    printf("\n");
    //nutrient_allprefixed(tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    print_tree(tree);

    nutrient_sync(tree);
    nutrient_close(tree);

    return 0;
}

int test_cidr()
{
    struct nutrient_tree *tree;
    uint8_t ip1[4] = { 10, 0, 0, 0 };
    uint8_t ip2[4] = { 10, 0, 3, 0 };
    uint8_t ip3[4] = { 10, 0, 3, 128 };
    uint8_t ip4[4] = { 129, 7, 6, 8 };

    uint8_t ip5[4] = { 10, 0, 3, 1 };
    uint8_t ip6[4] = { 10, 0, 3, 137 };
    uint8_t ip7[4] = { 10, 8, 0, 0 };
    uint8_t ip8[4] = { 11, 0, 0, 0 };

    uint8_t ip1_str[33];
    uint8_t ip2_str[33];
    uint8_t ip3_str[33];
    uint8_t ip4_str[33];
    uint8_t ip5_str[33];
    uint8_t ip6_str[33];
    uint8_t ip7_str[33];
    uint8_t ip8_str[33];

    const uint8_t * prefix;
    const uint8_t * value;
    uint32_t prefix_len;
    uint32_t value_len;
    

    ipv4_cidr_pack(ip1, 8, ip1_str);
    ipv4_cidr_pack(ip2, 24, ip2_str);
    ipv4_cidr_pack(ip3, 25, ip3_str);
    ipv4_cidr_pack(ip4, 32, ip4_str);
    ipv4_pack(ip5, ip5_str);
    ipv4_pack(ip6, ip6_str);
    ipv4_pack(ip7, ip7_str);
    ipv4_pack(ip8, ip8_str);

    ipv4_cidr_unpack(ip1_str, 8, ip1);
    ipv4_cidr_unpack(ip2_str, 24, ip2);
    ipv4_cidr_unpack(ip3_str, 25, ip3);
    ipv4_cidr_unpack(ip4_str, 32, ip4);

    unlink("cidr");
    tree = nutrient_create("cidr");

    nutrient_insert(tree, ip1_str, 8,  (uint8_t *) "10.0.0.0/8", 11);
    nutrient_insert(tree, ip2_str, 24, (uint8_t *) "10.0.3.0/24", 12);
    nutrient_insert(tree, ip3_str, 25, (uint8_t *) "10.0.3.128/25", 14);
    nutrient_insert(tree, ip4_str, 32, (uint8_t *) "129.7.6.8/32", 13);

    printf("All CIDRs:\n");
    nutrient_allprefixed(tree, (uint8_t *) "", 0, cidr_cb, NULL);

    printf("Longest match:\n");

    printf("129.7.6.8 (%s): %d ", ip4_str, nutrient_find_longest_prefix(tree, ip4_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("10.0.3.1 (%s): %d ", ip5_str, nutrient_find_longest_prefix(tree, ip5_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("10.0.3.137 (%s): %d ", ip6_str, nutrient_find_longest_prefix(tree, ip6_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("10.8.0.0 (%s): %d ", ip7_str, nutrient_find_longest_prefix(tree, ip7_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    printf("11.0.0.0 (%s): %d ", ip6_str, nutrient_find_longest_prefix(tree, ip8_str, 32, &prefix, &prefix_len, &value, &value_len));
    printf("%s\n", value);

    print_tree(tree);

    printf("\n\n");
    
#if 0
    nutrient_insert(tree, ip5_str, 32, "10.0.3.1/32", 12);
    print_tree(tree);
    printf("\n\n");
    nutrient_allprefixed(tree, "000010100000000000000011", 24, cidr_cb, NULL);
#endif

    nutrient_sync(tree);
    nutrient_close(tree);

    return 0;
}


int main(int argc, char **argv)
{
    test_cidr();

//    test_critbit0();
    printf("\n");

    return 0;
}
