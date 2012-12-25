#include "util.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void * xmalloc(size_t size)
{
    void * res = malloc(size);
    if (!res)
        errx(1, "malloc");
    return res;
}

void * xrealloc(void * ptr, size_t size)
{
    void * res = realloc(ptr, size);
    if (!res && size)
        errx(1, "realloc");
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


void slurp_path(const char * path, unsigned char * * restrict buffer,
                size_t * restrict offset, size_t * restrict size)
{
    int fd = checkz(open(path, O_RDONLY), "open input");
    slurp_file(fd, buffer, offset, size);
    close(fd);
}


void dump_file(int file, const void * data, size_t len)
{
    const unsigned char * start = data;
    const unsigned char * end = start + len;
    for (const unsigned char * p = start; p != end;)
        p += checkz(write(file, p, end - p), "writing output");
}


const unsigned char * bitfile_find_stream(const unsigned char * p,
                                          const unsigned char ** pend)
{
    const unsigned char * end = *pend;

    static unsigned char header[] = {
        0, 9, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0, 0, 1 };

    if (end - p < sizeof(header) || memcmp(p, header, sizeof header) != 0)
        errx(1, "File does not start with header.\n");

    p += sizeof header;

    while (1) {
        if (end - p < 3)
            errx(1, "EOF, no data.\n");

        int section = *p;
        if (section == 'e')
            // This is it.
            break;

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

    unsigned len = (p[1] << 24) + p[2] * 65536 + p[3] * 256 + p[4];
    p += 5;

    if (end - p < len)
        errx(1, "Data length overruns file.\n");

    if (end - p > len)
        warnx("Data followed by %zu trailing bytes.\n", end - p - len);

    fprintf(stderr, "Data block is %u bytes long.\n", len);

    *pend = p + len;
    return p;
}
