#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>

#include "nutrient.h"

int dump_key_value(const uint8_t *key, uint32_t key_len, const uint8_t *value,
                   uint32_t value_len, void *arg)
{
    printf("+%" PRIu32 ",%" PRIu32 ":", key_len, value_len);
    fwrite(key, key_len, 1, stdout);
    printf("->");
    fwrite(value, value_len, 1, stdout);
    printf("\n");

    return 1;
}

void usage()
{
    fprintf(stderr, "usage: nutrient_dump db-file\n"); 
    _exit(1);
}

int main(int argc, char * argv[])
{
    if (argc != 2)
        usage();

    critbit0_tree * tree = critbit0_open(argv[1]);
    if (tree == NULL) {
        usage();
    }

    critbit0_allprefixed(tree, (uint8_t *) "", 0, dump_key_value, NULL);     
    
    critbit0_close(tree);

    return 0;
}
