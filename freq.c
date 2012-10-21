// Measure the frequency of a given clock...
// Returns frequency in kHz.

#include "freq.h"
#include "monkey.h"
#include "registers.h"

#include <stddef.h>

static int freq_mon (unsigned clock, unsigned count)
{
    *FREQ_MON = clock * 16777216 + 0x800000 + count;
    for (int i = 0; i != 100; ++i)
        asm volatile ("");              // Delay a bit to let clocks start.
    while (true) {
        unsigned f = *FREQ_MON;
        if (!(f & (1 << 23)))
            return f;                   // Finished.
        if ((f & 0x7fffff) == count)
            return -1;                  // Nothing happening.
    }
}


int frequency (unsigned clock, unsigned multiplier)
{
    int fm = freq_mon (clock, 511);
    if (fm < 0)
        return fm;
    unsigned r = 511 - (fm & 511);
    if (r != 511 && r > 3) {
        fm = freq_mon (clock, r - 3);
        r = r - 3 - (fm & 511);
    }
    unsigned f = (fm >> 9) & 16383;
    return (f * 24 * multiplier + r) / (2 * r);
}


static const char * const clock_names[] = {
    "32 kHz",
    "IRC",
    "ENET_RX_CLK",
    "ENET_TX_CLK",
    "GP_CLKIN",
    NULL,
    "Crystal",
    "PLL0USB",
    "PLL0AUDIO",
    "PLL1",
    NULL, NULL,
    "IDIVA",
    "IDIVB",
    "IDIVC",
    "IDIVD",
    "IDIVE"
};


void clock_report (void)
{
    printf ("Frequencies...\n");
    for (int i = 0; i != sizeof clock_names / sizeof clock_names[0]; ++i) {
        const char * name = clock_names[i];
        if (name == NULL)
            continue;
        const char * dots = "...........";
        for (const char * p = clock_names[i]; *p && *dots; ++p, ++dots);
        int f = frequency (i, 1000);
        if (f >= 0)
            printf ("%s%s: %6u kHz\n", name, dots, f);
    }
    unsigned base_m4 = *((v32 *) 0x4005006c) >> 24;
    if (base_m4 < sizeof clock_names / sizeof clock_names[0]
        && clock_names[base_m4])
        printf ("CPU is %s.\n", clock_names[base_m4]);
}
