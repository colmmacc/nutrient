#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#include "nutrient.h"

void usage()
{
    fprintf(stderr, "usage: nutrient_patch input-file db-file\n"); 
    _exit(1);
}

void error(const char * message)
{
    fprintf(stderr, "nutrient_patch error: %s\n", message);
    _exit(1);
}

int main(int argc, char * argv[])
{
    if (argc != 3)
        usage();

    struct nutrient_tree * tree = nutrient_create(argv[2]);
    if (tree == NULL)
    {
        usage();
    }

    if (strcmp(argv[1], "-") == 0) {
        argv[1] = "/dev/stdin";
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        usage();
    }

    char operation;
    char c;
    uint8_t * key_data = NULL; 
    uint8_t * value_data = NULL;
    char seperator[2];

    for (;;) {
        uint32_t key_len = 0;
        uint32_t value_len = 0;

        int r = read(fd, &operation, 1);

        if (r == 0) {
            break;
        }
        if (r < 0) {
            error("Couldn't read operation");
        }

        /* Read the key length */
        do {
            r = read(fd, &c, 1);
            if (r != 1) {
                error("Couldn't read key length");
            }

            if (!isdigit(c)) {
                if (c != ',') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            key_len *= 10;
            key_len += c - '0';
        } while (1);
        
        /* Read the value length */
        do {
            r = read(fd, &c, 1);
            if (r != 1) {
                error("Couldn't read value length");
            }

            if (!isdigit(c)) {
                if (c != ':') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            value_len *= 10;
            value_len += c - '0';
        } while (1);

        /* Read the key data */
        key_data = realloc(key_data, key_len);
        if (key_data == NULL) {
            error("Out of memory");
        }
        if (read(fd, key_data, key_len) < key_len)
        {
            error("Couldn't read key data");
        }

        /* Read the seperator '->' */
        if (read(fd, seperator, 2) < 2)
        {
            error("Couldn't read seperator");
        }
        if (memcmp(seperator, "->", 2)) 
        {
            error("Malformed seperator");
        }

        /* Read the value data */
        value_data = realloc(value_data, value_len);
        if (value_data == NULL) {
            error("Out of memory");
        }
        if (read(fd, value_data, value_len) < value_len)
        {
            error("Couldn't read value data");
        }

        /* Insert or delete the record */
        switch (operation) {
            case '+': nutrient_insert(tree, key_data, key_len, value_data, value_len);
                      break;

            case '-':
            default:
                      usage();
                      break;
        }

        r = read(fd, &c, 1);
        if (r != 1) {
            error("Couldn't read EOL");
            usage();
        }

        if (c != '\n') {
            error("Malformed input");
        }
    }

    nutrient_sync(tree);
    nutrient_close(tree);

    return 0;
}


