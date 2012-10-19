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
    return f;
}


unsigned frequency (unsigned clock, unsigned multiplier)
{
    unsigned fm = freq_mon (clock, 511);
    unsigned r = 511 - (fm & 511);
    if (r != 511 && r > 3) {
        fm = freq_mon (clock, r - 3);
        r = r - 3 - (fm & 511);
    }
    unsigned f = (fm >> 9) & 16383;
    return (f * 24 * multiplier + r) / (2 * r);
}
