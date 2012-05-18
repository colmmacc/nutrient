#include <string.h>

#include "nutrient_util.h"

void uint64_pack(uint64_t u, char *p)
{
    memcpy(p, &u, sizeof(uint64_t));
}

void uint64_unpack(const char *p, uint64_t * u)
{
    memcpy(u, p, sizeof(uint64_t));
}

void uint32_pack(uint32_t u, char *p)
{
    memcpy(p, &u, sizeof(uint32_t));
}

void uint32_unpack(const char *p, uint32_t * u)
{
    memcpy(u, p, sizeof(uint32_t));
    //printf("%02x %02x %02x %02x %lu\n", p[0], p[1], p[2], p[3], *u);
}
