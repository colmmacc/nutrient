#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#include "nutrient.h"
#include "nutrient_util.h"

void usage()
{
    fprintf(stderr, "usage: nutrient_cidrs input-file db-file\n"); 
    _exit(1);
}

void error(const char * message)
{
    fprintf(stderr, "nutrient_cidrs error: %s\n", message);
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

    for (;;) {
        uint8_t     cidr[4];
        uint8_t     length;
        char c;
        int r;

        /* Read the first quad  */
        cidr[0] = 0;
        do {
            r = read(fd, &c, 1);
            if (r == 0) {
                goto done;
            }
            if (r != 1) {
                error("Couldn't read key length");
            }

            if (!isdigit(c)) {
                if (c != '.') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            cidr[0] *= 10;
            cidr[0] += c - '0';
        } while (1);

         /* Read the second quad  */
        cidr[1] = 0;
        do {
            r = read(fd, &c, 1);
            if (r != 1) {
                error("Couldn't read key length");
            }

            if (!isdigit(c)) {
                if (c != '.') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            cidr[1] *= 10;
            cidr[1] += c - '0';
        } while (1);

         /* Read the third quad  */
        cidr[2] = 0;
        do {
            r = read(fd, &c, 1);
            if (r != 1) {
                error("Couldn't read key length");
            }

            if (!isdigit(c)) {
                if (c != '.') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            cidr[2] *= 10;
            cidr[2] += c - '0';
        } while (1);

         /* Read the fourth quad  */
        cidr[3] = 0;
        do {
            r = read(fd, &c, 1);
            if (r != 1) {
                error("Couldn't read key length");
            }

            if (!isdigit(c)) {
                if (c != '/') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            cidr[3] *= 10;
            cidr[3] += c - '0';
        } while (1);

        /* Read the length  */
        length = 0;
        do {
            r = read(fd, &c, 1);
            if (r != 1) {
                error("Couldn't read key length");
            }

            if (!isdigit(c)) {
                if (c != ':') {
                    error("Malformed key/value pair");
                }
        
                break;
            }

            length *= 10;
            length += c - '0';
        } while (1);

        int i = 0;
        char value[1024];

        while (read(fd, value + i, 1))
        {
            if (value[i] == '\n')
            {
                value[i] = '\0';
                break;
            } 

            if (++i == sizeof(value))
            {
                error("value string too long");
            }
        }

        /* Insert the cidr and its value */
        uint8_t cidr_key[32];
        ipv4_cidr_pack(cidr, length, cidr_key);

        nutrient_insert(tree, cidr_key, length, (uint8_t *) value, strlen(value));
    }

    done:
    nutrient_sync(tree);
    nutrient_close(tree);

    return 0;
}


