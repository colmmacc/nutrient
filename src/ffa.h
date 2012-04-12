#ifndef _FFA_H_
#define _FFA_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    int fd;
    void *base;
    uint64_t size;
} ffa_t;

ffa_t *ffa_create(const char *filename);
ffa_t *ffa_open(const char *filename);
uint64_t ffa_alloc(ffa_t * ffa, size_t size);
void ffa_sync(ffa_t * ffa);
void ffa_close(ffa_t * ffa);

#endif /* _FFA_H_ */
