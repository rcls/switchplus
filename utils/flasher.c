// First arg is flash code, second is payload.

#include "util.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

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

    int fd = checkz (open (argv[1], O_RDONLY), "open driver");
    slurp_file (fd, &buffer, &offset, &size);
    close (fd);

    // Round up to a multiple of 512.
    size_t payload = (offset + 511) & ~511;
    zero_extend (payload, &buffer, &offset, &size);

    fd = checkz (open (argv[2], O_RDONLY), "open payload");
    slurp_file (fd, &buffer, &offset, &size);
    close (fd);

    size_t rounded = (offset + 511) & ~511;
    zero_extend (rounded, &buffer, &offset, &size);

    uint32_t * p = (uint32_t *) buffer;
    p[63] = payload + 0x10000000;       // Use the ram address.
    p[62] = rounded - payload;

    dump_file (1, buffer, rounded);
    return 0;
}
