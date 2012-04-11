#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "critbit.h"

int allprefixed_cb(const char * key, uint32_t key_len, void * value, void * arg)
{
    write(STDOUT_FILENO, "key=", 4);
    write(STDOUT_FILENO, key, key_len);
    write(STDOUT_FILENO, " value=", 7);
    printf("%ld\n", (long int) value);

    return 1;
}

int main(int argc, char ** argv)
{
    critbit0_tree tree = { 0 };

    critbit0_insert(&tree, "colm", 5, (void *) 657);
    critbit0_insert(&tree, "columnar", 9, (void *) 822);
    critbit0_insert(&tree, "veronica", 9, (void *) 123);
    critbit0_insert(&tree, "colm", 5, (void *) 456);

    critbit0_allprefixed(&tree, "col", 3, allprefixed_cb, NULL);

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);

    critbit0_delete(&tree, "colm", 4);

    printf("\n");
    critbit0_allprefixed(&tree, "", 0, allprefixed_cb, NULL);
    printf("\n");

    printf("veronica: %d\n", (long int) critbit0_find(&tree, "veronica", 9));
    printf("bat:  %d\n", critbit0_find(&tree, "bat", 4));

    critbit0_delete(&tree, "veronica", 9);

    printf("veronica: %d\n", critbit0_find(&tree, "veronica", 9));
    printf("bat:  %d\n", critbit0_find(&tree, "bat", 4));

    return 0;
}
