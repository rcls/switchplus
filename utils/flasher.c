// First arg is flash code, second is payload.

#include "util.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void zero_extend (size_t to, unsigned char **buffer,
                         size_t * offset, size_t * size)
{
    if (to > *size) {
        *buffer = xrealloc (*buffer, to);
        *size = to;
    }

    assert (to >= *offset);
    memset (*buffer + *offset, 0, to - *offset);
    *offset = to;
}


int main (int argc, const char * argv[])
{
    if (argc != 3)
        errx(1, "Usage: %s <flash> <payload>\n", argv[0]);

    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    slurp_path (argv[1], &buffer, &offset, &size);

    // Round up to a multiple of 512.
    size_t payload = (offset + 511) & ~511;
    zero_extend (payload, &buffer, &offset, &size);

    slurp_path (argv[2], &buffer, &offset, &size);

    size_t rounded = (offset + 511) & ~511;
    zero_extend (rounded, &buffer, &offset, &size);

    uint32_t * p = (uint32_t *) buffer;
    p[63] = payload + 0x10000000;       // Use the ram address.
    p[62] = rounded - payload;

    dump_file (1, buffer, rounded);
    return 0;
}
