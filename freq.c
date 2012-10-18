// Measure the frequency of a given clock...
// Returns frequency in kHz.

#include "freq.h"
#include "registers.h"

static unsigned freq_mon (unsigned clock, unsigned count)
{
    *FREQ_MON = clock * 16777216 + 0x800000 + count;
    unsigned f;
    do
        f = *FREQ_MON;
    while (f & (1 << 23));
    return (f >> 9) & 16383;
}

unsigned frequency (unsigned clock, unsigned multiplier)
{
    // First do a rcnt count to 256; this should cover everything...
    unsigned f = freq_mon (clock, 256);
    unsigned r = 511;
    if (f > 6144)
        r = 12288 * 256 / f;
    // Now do it for real...
    return (freq_mon (clock, r) * 24 * multiplier + r) / (2 * r);
}
