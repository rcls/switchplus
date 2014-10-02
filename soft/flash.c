// Flash a payload.

#include "registers.h"

void * start[64] __attribute__ ((section (".start"), externally_visible));
static void main (void);

// Start address of payload is start[63], number of bytes is start[62].
void * start[64] = {
    (void*) 0x10089fe0,
    main,
};

static void printc (char c)
{
    while (!(*USART3_LSR & 32));         // Wait for THR to be empty.
    *USART3_THR = c;
}

static void print (const char * s)
{
    while (*s)
        printc (*s++);
}

static void printh (unsigned v)
{
    for (int i = 7; i >= 0; --i)
        printc ("0123456789abcdef"[(v >> (i * 4)) & 15]);
}


static unsigned command (unsigned op, unsigned a, unsigned b,
                         unsigned c, unsigned d)
{
    print ("Command ");
    printh (op);
    printc (' ');
    printh (a);
    printc (' ');
    printh (b);
    printc (' ');
    printh (c);
    printc (' ');
    printh (d);

    typedef unsigned IAP_t (unsigned *, unsigned *);
    IAP_t * IAP = * (void**) 0x10400100;
    unsigned param[6];
    param[0] = op;
    param[1] = a;
    param[2] = b;
    param[3] = c;
    param[4] = d;
    unsigned r = IAP (param, param);
    print (" gives ");
    printh (r);
    print ("\r\n");
    return r;
}


static unsigned sector_command (unsigned op, unsigned s, unsigned b, unsigned c)
{
    return command (op, s, s, b, c);
}


static void die (const char * s)
{
    print (s);
    while (1);
}


static unsigned freq_mon (unsigned clock, unsigned count)
{
    *FREQ_MON = clock * 16777216 + 0x800000 + count;
    for (int i = 0; i != 100; ++i)
        asm volatile ("");              // Delay a bit to let clocks start.
    while (1) {
        unsigned f = *FREQ_MON;
        if (!(f & (1 << 23)))
            return f;                   // Finished.
        if ((f & 0x7fffff) == count)
            die ("Getting frequency failed\n"); // Nothing happening.
    }
}

// In kHz.
static unsigned frequency (unsigned clock)
{
    unsigned fm = freq_mon (clock, 511);
    unsigned r = 511 - (fm & 511);
    if (r != 511 && r > 3) {
        fm = freq_mon (clock, r - 3);
        r = r - 3 - (fm & 511);
    }
    unsigned f = (fm >> 9) & 16383;
    return (f * 24000 + r) / (2 * r);
}


static int cpu_frequency(void)
{
    unsigned base_m4 = *BASE_M4_CLK >> 24;
    return frequency (base_m4);
}


void main (void)
{
    unsigned size = (unsigned) start[62];

    unsigned sectors;
    if (size < 65536)
        sectors = (size + 8191) >> 13;
    else
        sectors = ((size + 65535) >> 16) + 7;

    unsigned bank = 0;
    unsigned cpu_freq = cpu_frequency();

    for (int i = 0; i != sectors; ++i) {
        print ("Blanking...\r\n");
        /* if (sector_command (53, i, bank, 0) != 0) { // Blank check. */
        print (" not blank...\r\n");
        sector_command (50, i, bank, 0); // Prepare for write.
        sector_command (52, i, cpu_freq, bank); // Erase.
        if (sector_command (53, i, bank, 0) != 0)
            die ("Cannot blank\r\n");
        /* } */
    }

    // Now do the writes...
    size = (size + 511) & ~511;
    unsigned source = (unsigned) start + (unsigned) start[63];
    unsigned dest;
    if (bank == 0)
        dest = 0x1a000000;
    else
        dest = 0x1b000000;

    while (size > 0) {
        print ("Writing...\r\n");
        // Sigh... we need the sector number.
        unsigned offset = dest & 0xffffff;
        unsigned sector;
        if (offset < 65536)
            sector = offset >> 13;
        else
            sector = (offset >> 16) + 7;
        sector_command (50, sector, bank, 0); // Prepare.

        unsigned amount = 512;
        if (size > 1024)
            amount = 1024;
        if (size > 4096)
            amount = 4096;

        if (command (51, dest, source, amount, cpu_freq) != 0)
            die ("Cannot write\r\n");

        source += amount;
        dest += amount;
        size -= amount;
    }

    // Compare.
    size = (unsigned) start[62];
    size = (size + 511) & ~511;
    source = (unsigned) start[63];
    if (bank == 0)
        dest = 0x1a000000;
    else
        dest = 0x1b000000;
    if (command (56, dest, source, size, 0) != 0)
        die ("Compare fails\r\n");

    print ("Make boot....\r\n");
    if (command (60, bank, cpu_freq, 0, 0) != 0)
        die ("Cannot make boot\r\n");

    print ("Done\r\n");
    while (1) {
        RESET_CTRL[1] = 0xfffffff;
        RESET_CTRL[0] = 0xfffffff;
    }
}
