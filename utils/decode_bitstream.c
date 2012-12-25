#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int main (int argc, const char * const argv[])
{
    if (argc != 2)
        errx(1, "Usage: decode_bitstream <bitfile>\n");

    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    int fd = checkz(open(argv[1], O_RDONLY), "open input");
    slurp_file(fd, &buffer, &offset, &size);
    close(fd);

    const unsigned char * end = buffer + offset;
    const unsigned char * p = find_data (buffer, &end);

    const unsigned char * aa = memchr (p, 0xaa, end - p);
    if (aa == NULL || end - aa < 4
        || aa[1] != 0x99 || aa[2] != 0x55 || aa[3] != 0x66)
        errx(1, "No sync word in file.\n");

    for (; p != aa; ++p)
        if (*p != 0xff)
            errx(1, "No sync word in file.\n");

    p += 4;
    while (end - p >= 2) {
        unsigned h = p[0] * 256 + p[1];
        unsigned len = h & 31;
        printf("Type %d op %d reg %2d words %2d",
               h >> 13, (h >> 11) & 3, (h >> 5) & 31, h & 31);
        p += 2;
        if (end - p < len * 2)
            errx(1, "Data overruns.\n");
        for (unsigned i = 0; i != len; ++i, p += 2)
            printf (" %04x", p[0] * 256 + p[1]);
        if ((h >> 13) != 2) {
            printf("\n");
            continue;
        }
        if (end - p < 4)
            errx(1, "Type 2 word count overruns.\n");
        len = (p[0] << 24) + p[1] * 65536 + p[2] * 256 + p[3];
        p += 4;
        if (len & 0xf0000000)
            errx(1, "Type 2 word count too big.\n");
        len += 2;
        if (end - p < len * 2)
            errx(1, "Type 2 data overruns.\n");
        printf(" payload %u words\n", len);
        p += len * 2;
    }
    if (end != p)
        errx(1, "Trailing data.\n");
}
