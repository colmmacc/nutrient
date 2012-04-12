#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "ffa.h"

ffa_t *ffa_create(const char *filename)
{
    ffa_t *ret;

    ret = malloc(sizeof(ffa_t));
    if (ret == NULL) {
        return NULL;
    }

    ret->size = 0;
    ret->fd = open(filename, O_CREAT | O_RDWR, 0643);

    if (ret->fd < 0) {
        return NULL;
    }

    return ret;
}

ffa_t *ffa_open(const char *filename)
{
    struct stat st;
    ffa_t *ret;

    ret = malloc(sizeof(ffa_t));
    if (ret == NULL) {
        return NULL;
    }

    ret->fd = open(filename, O_RDONLY);
    if (ret->fd < 0) {
        return NULL;
    }

    if (fstat(ret->fd, &st) < 0) {
        return NULL;
    }

    ret->size = st.st_size;

    ret->base = mmap(NULL, ret->size, PROT_READ, MAP_PRIVATE, ret->fd, 0);
    if (ret->base == NULL) {
        close(ret->fd);
        return NULL;
    }

    return 0;
}

uint64_t ffa_alloc(ffa_t * ffa, size_t size)
{
    char allzeroes[1024] = { 0 };
    int written = 0;

    /* Only allocate aligned segments */
    if (size % (sizeof(void *))) {
        size += sizeof(void *) - (size % (sizeof(void *)));
    }

    while (written < size) {
        if (size - written > sizeof(allzeroes)) {
            write(ffa->fd, allzeroes, sizeof(allzeroes));
            written += sizeof(allzeroes);
        }
        else {
            write(ffa->fd, allzeroes, size - written);
            written += size - written;
        }
    }

    /* Unmap the old data */
    munmap(ffa->base, ffa - size);

    ffa->size += size;

    /* Map the new data */
    ffa->base =
        mmap(ffa->base, ffa->size, PROT_READ | PROT_WRITE, MAP_SHARED,
             ffa->fd, 0);

    return ffa->size - size;
}

void ffa_sync(ffa_t * ffa)
{
    msync(ffa->base, ffa->size, MS_SYNC);
}

void ffa_close(ffa_t * ffa)
{
    munmap(ffa->base, ffa->size);
    close(ffa->fd);
}
