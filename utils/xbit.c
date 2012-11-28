// Read a xilinx .bit file and spit it out as hex for our jtag.

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

void dump_data(const unsigned char * p, const unsigned char * end)
{
    if (end - p < 5)
        errx(1, "EOF, no data length.\n");

    unsigned len = (p[1] << 24) + p[2] * 65536 + p[3] * 256 + p[4];
    p += 5;

    if (end - p < len)
        errx(1, "Data length overruns file.\n");

    if (end - p > len)
        warnx("Data followed by %zu trailing bytes.\n", end - p - len);

    fprintf(stderr, "Data block is %u bytes long.\n", len);

    printf ("\033\njp\n");

    for (unsigned i = 0; i < len; ++i) {
        printf ("%02x", p[i]);
        if ((i & 31) == 31)
            printf ("\n");
    }
    if (len & 31)
        printf ("\n");

    printf (".\n");

    return;
}


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

    unsigned char header[] = {
        0, 9, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0, 0, 1 };

    if (offset < sizeof(header) || memcmp(buffer, header, sizeof(header)) != 0)
        errx(1, "File does not start with header.\n");

    const unsigned char * p = buffer + sizeof(header);
    const unsigned char * end = buffer + offset;

    while (p != end) {
        if (end - p < 3)
            errx(1, "EOF, no data.\n");

        int section = *p;
        if (section == 'e') {
            // This is it.
            dump_data(p, end);
            return 0;
        }

        if (section < 'a' || section > 'd')
            errx(1, "Section 0x%02x is unknown\n", section);

        int len = p[1] * 256 + p[2];
        p += 3;
        if (end - p < len)
            errx(1, "Section '%c' length %u overruns...\n", section, len);

        static const char * const tags[] = {
            "Design", "Part", "Date", "Time"
        };
        fprintf(stderr, "%s\t: %.*s\n", tags[section - 'a'], len, p);
        p += len;
    }
}
