#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "nutrient.h"
#include "critbit.h"
#include "ffa.h"

int allprefixed_cb(const char *key, uint32_t key_len, const char *value, uint32_t value_len,
                   void *arg)
{
    printf("key_len=%u value_len=%u key=%s value=%s\n", key_len, value_len, key, value);
    return 1;
}

int test_critbit0()
{
    critbit0_tree tree = { 0 };
    const char * value;
    uint32_t value_len;

    critbit0_insert(&tree, "colm", 5, "657", 4);
    critbit0_insert(&tree, "columnar", 9, "822", 4);
    critbit0_insert(&tree, "veronica", 9, "123", 4);
    critbit0_insert(&tree, "veronica", 9, "456", 4);
    critbit0_insert(&tree, "colm", 5, "456", 4);

    critbit0_allprefixed(&tree, "col", 3, allprefixed_cb, NULL);

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);

    critbit0_delete(&tree, "colm", 4);

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    printf("veronica: %d ", critbit0_find(&tree, "veronica", 9, &value, &value_len));
    printf("value_len: %u value: %s\n", value_len, value);
    printf("bat:  %d\n", critbit0_find(&tree, "bat", 4, &value, &value_len));

    critbit0_delete(&tree, "veronica", 9);

    printf("veronica: %d\n", critbit0_find(&tree, "veronica", 9, &value, &value_len));
    printf("bat:  %d\n", critbit0_find(&tree, "bat", 4, &value, &value_len));

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    return 0;
}

int test_ffa()
{
    struct ffa  *handle;
    char * offset;

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

int main(int argc, char **argv)
{
    test_critbit0();

    test_ffa();

    return 0;
}
