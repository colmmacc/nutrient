#pragma once

#include <stdint.h>

void uint64_pack(uint64_t u, char *p);
void uint64_unpack(const char *p, uint64_t * u);

void uint32_pack(uint32_t u, char *p);
void uint32_unpack(const char *p, uint32_t * u);
