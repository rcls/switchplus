#ifndef UTIL_H_
#define UTIL_H_

#include <err.h>
#include <stddef.h>
#include <unistd.h>

void * xmalloc(size_t size) __attribute__((__malloc__, __warn_unused_result__));

void * xrealloc(void * ptr, size_t size)
    __attribute__((__warn_unused_result__));

void slurp_file(int file, unsigned char * * restrict buffer,
                size_t * restrict offset, size_t * restrict size);

void dump_file(int file, const void * data, size_t len);

inline ssize_t checkz(ssize_t r, const char * w)
{
    if (r < 0)
        err(1, "%s", w);
    return r;
}

#endif
