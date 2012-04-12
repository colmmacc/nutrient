#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "nutrient.h"
#include "critbit.h"
#include "ffa.h"

int allprefixed_cb(const char *key, uint32_t key_len, uint64_t value,
                   void *arg)
{
    write(STDOUT_FILENO, "key=", 4);
    write(STDOUT_FILENO, key, key_len);
    write(STDOUT_FILENO, " value=", 7);
    printf("%ld\n", (long int) value);

    return 1;
}

int test_critbit0()
{
    critbit0_tree tree = { 0 };
    uint64_t value;

    critbit0_insert(&tree, "colm", 5, 657);
    critbit0_insert(&tree, "columnar", 9, 822);
    critbit0_insert(&tree, "veronica", 9, 123);
    critbit0_insert(&tree, "colm", 5, 456);

    critbit0_allprefixed(&tree, "col", 3, allprefixed_cb, NULL);

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);

    critbit0_delete(&tree, "colm", 4);

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    printf("veronica: %d ", critbit0_find(&tree, "veronica", 9, &value));
    printf("value: %d\n", (int) value);
    printf("bat:  %d\n", critbit0_find(&tree, "bat", 4, &value));

    critbit0_delete(&tree, "veronica", 9);

    printf("veronica: %d\n", critbit0_find(&tree, "veronica", 9, &value));
    printf("bat:  %d\n", critbit0_find(&tree, "bat", 4, &value));

    return 0;
}

int test_ffa()
{
    ffa_t *ffa;
    uint64_t offset;

    ffa = ffa_create("stupid");

    offset = ffa_alloc(ffa, 10);
    printf("offset: %d\n", offset);

    strcpy(((char *) ffa->base) + offset, "colm");

    offset = ffa_alloc(ffa, 20);
    printf("offset: %d\n", offset);
    strcpy(((char *) ffa->base) + offset, "veronica");

    offset = ffa_alloc(ffa, 30);
    printf("offset: %d\n", offset);
    strcpy(((char *) ffa->base) + offset, "foobar");

    ffa_sync(ffa);
    ffa_close(ffa);

    return 0;
}

int main(int argc, char **argv)
{
    test_critbit0();

    test_ffa();

    return 0;
}
