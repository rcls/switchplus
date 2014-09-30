#include "util.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

// Add the header required by the LPC4357 boot loader.
// See table 5.3.4 (page 52) of the user-manual.
int main (int argc, char * const argv[])
{
    unsigned char * buffer = NULL;
    size_t length = 0;
    size_t size = 0;

    slurp_file(0, &buffer, &length, &size);

    size_t rounded = (length + 511) & ~511;
    buffer = xrealloc (buffer, rounded);

    memset (buffer + length, 0, rounded - length);

    // bits 5:0    : 0x1a - no AES encryption.
    // bits 7:6    : 11 - no hash is used.
    // bits 13:8   : Reserved all ones.
    // bits 15:14  : AES control, don't care for us.
    // bits 31:16  : Size of payload in 512 byte blocks.
    // bits 95:32  : Hash value, not used, all ones.
    // bits 127:96 : Reserved, all ones.
    uint32_t header[4] = {
        ((rounded >> 9) << 16) + 0xffda,
        -1, -1, -1
    };

    assert (sizeof header * 8 == 128);

    dump_file (1, header, sizeof header);
    dump_file (1, buffer, rounded);
}
