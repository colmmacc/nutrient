#pragma once

#include <stdint.h>

void uint64_pack(uint64_t u, uint8_t *p);
void uint64_unpack(const uint8_t *p, uint64_t * u);

void uint32_pack(uint32_t u, uint8_t *p);
void uint32_unpack(const uint8_t *p, uint32_t * u);

int ipv4_cidr_pack(uint8_t * ip, uint8_t bits, uint8_t * packed);
int ipv6_cidr_pack(uint8_t * ip, uint8_t bits, uint8_t * packed);

int ipv4_cidr_unpack(uint8_t * packed, uint8_t bits, uint8_t * ip);
int ipv6_cidr_unpack(uint8_t * packed, uint8_t bits, uint8_t * ip);
