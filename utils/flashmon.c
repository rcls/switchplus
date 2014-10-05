// Arg is payload, outputs monitor script.

#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void * zero_extend (void * p, size_t old_size, size_t new_size)
{
    p = xrealloc(p, new_size);
    if (old_size < new_size)
        memset(p + old_size, 0, new_size - old_size);
    return p;
}


static void write_block(const unsigned char * p,
                        uint32_t address, int length)
{
    uint32_t csum = ~0;
    for (int i = 0; i < length; ++i)
        csum = csum * 253 + p[i];

    printf("flash %08x %03x %08x\n", address, length, csum);
    for (int i = 0; i < length; ++i)
        printf("%02x", p[i]);

    printf("\n");
}


int main(int argc, char * argv[])
{
    if (argc != 2)
        errx(1, "Usage: %s <payload>\n", argv[0]);

    unsigned char * buffer = NULL;
    size_t offset = 0;
    size_t size = 0;

    slurp_path (argv[1], &buffer, &offset, &size);

    // Round up to a multiple of 512.
    size_t length = (offset + 511) & ~511;
    buffer = zero_extend(buffer, offset, length);

    uint32_t base = ((uint32_t * ) buffer)[1] & ~0x3ffff;
    if ((base & 0xfef80000) != 0x1a000000)
        errx(1, "Base address %#08x is not valid\n", base);

    // See if we need to be careful about CRP and validity.
    size_t skip = 0;
    if ((base & 0x00ffffff) == 0 && length >= 1024) {
        skip = 512;
        uint32_t crp = * (uint32_t *) (buffer + 0x2fc);
        if (crp == 0x12345678
            || crp == 0x87654321
            || crp == 0x43218765
            || crp == 0x4e697370)
            errx(1, "File enables CRP");

        // Now update the checksum...
        uint32_t cs = 0;
        for (int i = 0; i < 7; ++i)
            cs -= ((uint32_t *) buffer)[i];
        ((uint32_t *) buffer)[7] = cs;
    }

    // Enter monitor...
    printf("\n\x1b\n\x1b\nM");

    // Unlock.
    printf("unlock\n");

    // Round the base address down to a sector.
    uint32_t erase_start = base & ~8191;
    if (erase_start & (0x00ff0000))
        erase_start = base & ~65535;

    // Round the end address up to a sector boundary.
    uint32_t erase_end = (base + length + 8191) & ~8191;
    if (erase_end & (0x00ff0000))
        erase_end = (erase_end + 65535) & ~65535;

    //printf("erase %08x %06x\n", erase_start, erase_end - erase_start);

    for (size_t i = skip; i < length;) {
        // Do the largest of 512, 1024, 4096 that matches alignment
        // and does not overrun.
        size_t n;
        if ((i & 4095) == 0 && 4096 <= length - i)
            n = 4096;
        else if ((i & 1023) == 0 && 1024 <= length - i)
            n = 1024;
        else if ((i & 511) == 0 && 512 <= length - i)
            n = 512;
        else
            errx(1, "Huh? Dag at end, %#06zx to %#06zx", i, length);

        write_block(buffer + i, base + i, n);
        i += n;
    }

    if (skip)
        write_block(buffer, base, skip);

    return 0;
}
