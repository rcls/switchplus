// Read a binary file on stdin, and output commands to write to ram and run.

#include "util.h"

#include <err.h>
#include <stdio.h>

static size_t do_line (const unsigned char * buffer, size_t start, size_t end)
{
    char data[80];
    char * p = data;
    unsigned current = 0;
    unsigned bits = 0;
    if (end > start + 45)
        end = start + 45;
    for (size_t i = start; i < end; ++i) {
        current = current * 256 + buffer[i];
        bits += 8;
        do {
            unsigned value = (current >> (bits - 6)) & 63;
            if (value == 0)
                value += 64;
            *++p = value + 32;
            bits -= 6;
        }
        while (bits >= 6);
    }

    if (bits > 0) {
        unsigned value = (current << (8 - bits)) & 63;
        if (value == 0)
            value += 64;
        *++p = value + 32;
    }

    p[1] = 0;
    data[0] = (end - start) + 32;
    printf("%s\n", data);
    return end;
}


static size_t do_block (const unsigned char * buffer, size_t start, size_t end)
{
    size_t done = start;
    for (int lines = 0; lines < 20 && done < end; ++lines)
        done = do_line (buffer, done, end);

    int checksum = 0;
    for (size_t i = start; i < done; ++i)
        checksum += buffer[i];

    printf("%u\n", checksum);
    return done;
}


int main (int argc, char * const argv[])
{
    unsigned char * buffer = NULL;
    size_t length = 0;
    size_t size = 0;

    slurp_file(0, &buffer, &length, &size);

    if (size == 0)
        errx (1, "Nothing to do!");

    printf("A 0\n");                    // Turn off echo.
    printf("W %u %zu\n", 0x10000000, length);

    for (size_t done = 0; done < length; done = do_block(buffer, done, length));

    printf("G %u T\n", 0x10000000);

    return 0;
}
