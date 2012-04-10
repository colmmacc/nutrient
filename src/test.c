#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "critbit.h"

int allprefixed_cb(const char * elem, void * arg)
{
    printf("found: %s\n", elem);
    return 1;
}

int main(int argc, char ** argv)
{
    critbit0_tree tree = { 0 };

    critbit0_insert(&tree, "colm", 5, NULL);
    critbit0_insert(&tree, "columnar", 9, NULL);
    critbit0_insert(&tree, "veronica", 9, NULL);
    critbit0_insert(&tree, "colm", 5, NULL);

/*
    critbit0_allprefixed(&tree, "col", allprefixed_cb, NULL);
    printf("\n");
    critbit0_allprefixed(&tree, "", allprefixed_cb, NULL);

    critbit0_delete(&tree, "colm", 4);

*/
    printf("\n");
    critbit0_allprefixed(&tree, "", allprefixed_cb, NULL);

    printf("veronica: %d\n", critbit0_contains(&tree, "veronica", 9));
    printf("bat:  %d\n", critbit0_contains(&tree, "bat", 4));

    critbit0_delete(&tree, "veronica", 9);

    printf("veronica: %d\n", critbit0_contains(&tree, "veronica", 9));
    printf("bat:  %d\n", critbit0_contains(&tree, "bat", 4));

    return 0;
}
