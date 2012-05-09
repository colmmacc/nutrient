#pragma once

#include <stdlib.h>
#include <stdint.h>

/*
** Open a flat-file allocator 
**
** @param filename  The file for storing data
** @return a pointer to a useable struct ffa structure on success,
**         or NULL on failure. errno will be set appropriately.
*/
struct ffa *ffa_create(const char *filename);
struct ffa *ffa_open(const char *filename);

/*
** Allocate memory from the flat-file allocator
**
** @param handle The struct ffa for the opened file
** @param size   Allocate size bytes of memory
**
** @return a pointer to the allocated memory,
**         or NULL on error. errno will be
**         set appropriately. 
*/
void * ffa_alloc(struct ffa * handle, size_t size);

/*
** De-allocate memory from the flat-file 
** allocator
** 
** @param handle The struct ffa for the opened file
** @param ptr    Pointer to the memory to be
**               de-allocated
** @param size   size bytes of memory will be
**               recorded as free
*/
int ffa_free(struct ffa * handle, void * ptr, size_t size);

int ffa_sync(struct ffa * handle);
int ffa_close(struct ffa * handle);
