#include "util.h"

#include <stdlib.h>
#include <unistd.h>

void * xmalloc(size_t size)
{
    void * res = malloc(size);
    if (!res)
        err(1, "malloc");
    return res;
}

void * xrealloc(void * ptr, size_t size)
{
    void * res = realloc(ptr, size);
    if (!res)
        err(1, "realloc");
    return res;
}


void slurp_file(int file, unsigned char * * restrict buffer,
                size_t * restrict offset, size_t * restrict size)
{
    int r;
    do {
        if (*offset >= *size) {
            *size += (*size / 2 & -8192) + 8192;
            *buffer = xrealloc(*buffer, *size);
        }
        r = checkz(read(file, *buffer + *offset, *size - *offset), "read");
        *offset += r;
    }
    while (r);
}
