#ifndef _FFA_H_
#define _FFA_H_

#include <stdlib.h>

typedef struct
{
    int    fd;
    size_t size;
    void   * ptr;
} ffa_t;

ffa_t * ffa_create(const char * filename, int size);
ffa_t * ffa_load(const char * filename);
void * ffa_alloc(ffa_t * ffa, size_t size);
void ffa_free(ffa_t * ffa, size_t size);
void ffa_sync(ffa_t * ffa);

#endif /* _FFA_H_ */
