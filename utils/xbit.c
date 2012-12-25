// Read a xilinx .bit file and spit it out as hex for our jtag.

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util.h"


int main (int argc, const char * const argv[])
{
    if (argc != 2)
        errx(1, "Usage: xbit <bitfile>\n");

    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    int fd = checkz(open(argv[1], O_RDONLY), "open input");
    slurp_file(fd, &buffer, &offset, &size);
    close(fd);

    const unsigned char * end = buffer + offset;
    const unsigned char * p = find_data (buffer, &end);

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
