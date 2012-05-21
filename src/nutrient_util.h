#pragma once

#include <stdint.h>

/*
** Pack an unsigned 64-bit int into 8 bytes of memory
**
** @param u     The unsigned 64-bit int
** @param p     Pointer to at least 8-bytes of memory
*/
void uint64_pack(uint64_t u, uint8_t * p);

/*
** Unpack an unsigned 64-bit int from 8 bytes of memory
**
** @param u     The unsigned 64-bit int
** @param p     Pointer to at least 8-bytes of memory
*/
void uint64_unpack(const uint8_t * p, uint64_t * u);

/*
** Pack an unsigned 32-bit int into 4 bytes of memory
**
** @param u     The unsigned 32-bit int
** @param p     Pointer to at least 8-bytes of memory
*/
void uint32_pack(uint32_t u, uint8_t * p);

/*
** Unpack an unsigned 32-bit int from 4 bytes of memory
**
** @param u     The unsigned 32-bit int
** @param p     Pointer to at least 4-bytes of memory
*/
void uint32_unpack(const uint8_t * p, uint32_t * u);

/*
** Pack an IPv4 address, in network byte order, into a
** string representation - which will be "bits" long
** and can be used to store in an nutrient trie 
** for performing longest-prefix match.
**
** @param ip     Memory representing an IP address, must be
**               at least (bits / 8) bytes long.
** @param bits   The length of the CIDR, e.g. /24
** @param packed Pointer to at least 'bits' bytes of memory
**
** @return 0 on success, -1 on error
*/
int ipv4_cidr_pack(uint8_t * ip, uint8_t bits, uint8_t * packed);
#define ipv4_pack(ip, packed) ipv4_cidr_pack( (ip) , 32, (packed))

/*
** Pack an IPv6 address, in network byte order, into a
** string representation - which will be "bits" long
** and can be used to store in an nutrient trie 
** for performing longest-prefix match.
**
** @param ip     Memory representing an IP address, must be
**               at least (bits / 8) bytes long.
** @param bits   The length of the CIDR, e.g. /64
** @param packed Pointer to at least 'bits' bytes of memory
**
** @return 0 on success, -1 on error
*/
int ipv6_cidr_pack(uint8_t * ip, uint8_t bits, uint8_t * packed);
#define ipv6_pack(ip, packed) ipv4_cidr_pack( (ip) , 128, (packed))

/*
** Unpack 'bits' of an IPv4 address, from a string representation, into
** (bits / 8) bytes of memory - in network byte order.
**
** @param packed    Pointer to at least 'bits' bytes of memory
** @param bits      The length of the CIDR
** @param ip        Pointer to at least bits/8 bytes of memory
**
** @return 0 on success, -1 on error
*/
int ipv4_cidr_unpack(uint8_t * packed, uint8_t bits, uint8_t * ip);

/*
** Unpack 'bits' of an IPv6 address, from a string representation, into
** (bits / 8) bytes of memory - in network byte order.
**
** @param packed    Pointer to at least 'bits' bytes of memory
** @param bits      The length of the CIDR
** @param ip        Pointer to at least bits/8 bytes of memory
**
** @return 0 on success, -1 on error
*/
int ipv6_cidr_unpack(uint8_t * packed, uint8_t bits, uint8_t * ip);
