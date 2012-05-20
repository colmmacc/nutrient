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

#define FFA_ERROR 0xFFFFFFFF

/*
** Allocate memory from the flat-file allocator
**
** @param handle The struct ffa for the opened file
** @param size   Allocate size bytes of memory
**
** @return offset to the allocated memory,
**         or FFA_ERROR on error. errno will be
**         set appropriately. 
*/
uint64_t ffa_alloc(struct ffa *handle, size_t size);

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
uint64_t ffa_free(struct ffa *handle, uint64_t offset, size_t size);

/*
** Sync all in-memory changes to disk
**
** @param handle The struct ffa for the opened file
**
** @return 0 on success, -1 on error
*/
int ffa_sync(struct ffa *handle);

/*
** Close the ffa file
**
** @param handle The struct ffa for the opened file
**
** @return 0 on success, -1 on error
*/
int ffa_close(struct ffa *handle);


uint64_t ffa_get_size(struct ffa *handle);

void *ffa_get_memory(struct ffa *handle, uint64_t offset);

int ffa_truncate(struct ffa *handle, uint64_t size);
