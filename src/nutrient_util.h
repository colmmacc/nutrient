#pragma once

#include <stdint.h>

void uint64_pack(uint64_t u, char *p);
void uint64_unpack(const char *p, uint64_t * u);

void uint32_pack(uint32_t u, char *p);
void uint32_unpack(const char *p, uint32_t * u);

int ipv4_cidr_pack(uint8_t * ip, uint8_t bits, char * packed);
int ipv6_cidr_pack(uint8_t * ip, uint8_t bits, char * packed);

int ipv4_cidr_unpack(char * packed, uint8_t bits, uint8_t * ip);
int ipv6_cidr_unpack(char * packed, uint8_t bits, uint8_t * ip);
