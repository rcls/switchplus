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


const unsigned char * skip_sync(const unsigned char * p,
                                const unsigned char * end)
{
    const unsigned char * aa = memchr (p, 0xaa, end - p);
    if (aa == NULL || end - aa < 4
        || aa[1] != 0x99 || aa[2] != 0x55 || aa[3] != 0x66)
        errx(1, "No sync word in file.\n");

    for (const unsigned char * q = p; q != aa; ++q)
        if (*q != 0xff)
            errx(1, "No sync word in file.\n");

    return aa + 4;
}


// Check writes to a register in a bitstream.
void check_reg_writes(const unsigned char * p,
                      const unsigned char * end,
                      unsigned reg, unsigned mask, unsigned expect)
{
    if (end != p && *p == 0xff)
        p = skip_sync(p, end);

    while (end - p >= 2) {
        unsigned h = p[0] * 256 + p[1];
        unsigned len = h & 31;
        p += 2;
        if (end - p < len * 2)
            errx(1, "Data overruns.\n");

        if ((h >> 13) == 1 && ((h >> 11) & 3) == 2
            && ((h >> 5) & 31) == reg) {
            if (len != 1)
                errx(1, "Reg %d write len %d is not 1.\n", reg, len);
            unsigned v = p[0] * 256 + p[1];
            if ((v & mask) != expect)
                errx(1, "Reg %d write %04x masked %04x is %04x not %04x.\n",
                     reg, v, mask, v & mask, expect);
            fprintf(stderr, "Reg %d write %04x\n", reg, v);
        }


        p += len * 2;
        if ((h >> 13) != 2)
            continue;

        if (end - p < 4)
            errx(1, "Type 2 word count overruns.\n");
        len = (p[0] << 24) + p[1] * 65536 + p[2] * 256 + p[3];
        p += 4;
        if (len & 0xf0000000)
            errx(1, "Type 2 word count too big.\n");
        len += 2;
        if (end - p < len * 2)
            errx(1, "Type 2 data overruns.\n");
        p += len * 2;
    }
    if (end != p)
        errx(1, "Trailing data.\n");
}
