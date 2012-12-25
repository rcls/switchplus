// Read a xilinx .bit file and spit it out as hex for our spirom programmer.

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

#define PAGE_LEN 264

int main (int argc, const char * const argv[])
{
    if (argc != 2)
        errx(1, "Usage: pbit <bitfile>\n");

    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    slurp_path(argv[1], &buffer, &offset, &size);

    // Make sure we have enough room for padding...
    if (size < offset + PAGE_LEN)
        buffer = xrealloc (buffer, offset + PAGE_LEN);

    const unsigned char * end = buffer + offset;
    const unsigned char * p = bitfile_find_stream (buffer, &end);

    check_reg_writes (p, end, 10, 3, 0);

    memset ((unsigned char *) end, 0xff, PAGE_LEN);

    unsigned pages = (end - p + PAGE_LEN - 1) / PAGE_LEN;

    printf ("\033\ns");

    for (unsigned i = 0; i < pages; ++i) {
        const unsigned char * pp = p + i * PAGE_LEN;
        printf ("p%x:", i);
        for (int i = 0; i < PAGE_LEN; ++i)
            printf ("%02x", pp[i]);
        printf ("\n");
    }

    printf ("sis.");

    return 0;
}
