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
}

static int cidr_pack(const uint8_t * ip, uint8_t bits, char * packed)
{
    int bit;

    for(bit = 0; bit < bits; bit++)
    {
        packed[bit] = !!(ip[ bit / 8 ] & (1 << (7 - (bit % 8)) )) + '0';
    }

    packed[bit] = 0;
    
    return 0;
}

int ipv4_cidr_pack(uint8_t * ip , uint8_t bits, char * packed)
{
    if (bits > 32)
    {
        return -1;
    }

    return cidr_pack(ip, bits, packed);
}

int ipv6_cidr_pack(uint8_t * ip, uint8_t bits, char * packed)
{
    if (bits > 128)
    {
        return -1;
    }

    return cidr_pack(ip, bits, packed);
}

static int cidr_unpack(char * packed, uint8_t bits, uint8_t * ip, uint8_t size)
{
    int bit;

    /* Zero the entire IP to start with */
    memset(ip, 0, size);

    for (bit = 0; bit < bits; bit++)
    {
        if (packed[bit] == '1') 
        {
            ip[ bit / 8 ] |= (1 << (7 - bit % 8));
        }
    }
    
    return 0;
}

int ipv4_cidr_unpack(char * packed, uint8_t bits, uint8_t * ip)
{
    if (bits > 32)
    {
        return -1;
    }

    return cidr_unpack(packed, bits, ip, 4);
}

int ipv6_cidr_unpack(char * packed, uint8_t bits, uint8_t * ip)
{
    if (bits > 128)
    {
        return -1;
    }

    return cidr_unpack(packed, bits, ip, 16);
}
