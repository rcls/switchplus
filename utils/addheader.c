#include "util.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

int main (int argc, char * const argv[])
{
    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    slurp_file(0, &buffer, &offset, &size);

    size_t rounded = (offset + 511) & ~511;
    buffer = xrealloc (buffer, rounded);

    memset (buffer + offset, 0, rounded - offset);

    uint32_t header[4] = {
        ((rounded >> 9) << 16) + 0xffda,
        -1, -1, -1
    };

    assert (sizeof header * 8 == 128);

    dump_file (1, header, sizeof header);
    dump_file (1, buffer, rounded);
}
