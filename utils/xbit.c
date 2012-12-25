// Read a xilinx .bit file and spit it out as hex for our jtag.

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "util.h"


int main (int argc, const char * const argv[])
{
    if (argc != 2)
        errx(1, "Usage: xbit <bitfile>\n");

    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    slurp_path(argv[1], &buffer, &offset, &size);

    const unsigned char * end = buffer + offset;
    const unsigned char * p = bitfile_find_stream (buffer, &end);

    check_reg_writes(p, end, 10, 2, 2);

    unsigned len = end - p;

    printf ("\033\njp\n");

    for (unsigned i = 0; i < len; ++i) {
        printf ("%02x", p[i]);
        if ((i & 31) == 31)
            printf ("\n");
    }
    if (len & 31)
        printf ("\n");

    printf (".\n");

    return 0;
}
