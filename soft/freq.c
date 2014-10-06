// Measure the frequency of a given clock...
// Returns frequency in kHz.

#include "freq.h"
#include "monkey.h"
#include "registers.h"

#include <stddef.h>

static int freq_mon (unsigned clock, unsigned count)
{
    *FREQ_MON = clock * 16777216 + 0x800000 + count;
    spin_for(100);                      // Delay a bit to let clocks start.
    while (true) {
        unsigned f = *FREQ_MON;
        if (!(f & (1 << 23)))
            return f;                   // Finished.
        if ((f & 0x7fffff) == count)
            return -1;                  // Nothing happening.
    }
}

// Measurement of PLL0USB.  If clocks are perfect it, will be 16000.
static unsigned calibration = 16000;


// We assume PLL0USB is running at 480 MHz.  We measure that; the result is used
// to adjust future calls to frequency().
static void calibrate (void)
{
    int pll = freq_mon (7, 400);
    unsigned m = (pll >> 9) & 16383;
    if ((pll & 511) || m < 15680 || m > 16320)
        m = 16000;
    calibration = m;
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
    f = f * 16000 * 12 / calibration;
    return (f * 2 * multiplier + r) / (2 * r);
}


int cpu_frequency (unsigned multiplier)
{
    calibrate();
    unsigned base_m4 = *((v32 *) 0x4005006c) >> 24;
    return frequency (base_m4, multiplier);
}


static const char * const clock_names[] = {
    "32 kHz", "IRC", "ENET_RX_CLK", "ENET_TX_CLK", "GP_CLKIN", NULL,
    "Crystal", "PLL0USB", "PLL0AUDIO", "PLL1", NULL, NULL,
    "IDIVA", "IDIVB", "IDIVC", "IDIVD", "IDIVE"
};


static const char * const output_names[] = {
    "USB0", "PERIPH", "USB1", "M4", "SPIFI", "SPI", "PHY_RX", "PHY_TX",
    "APB1", "APB3", "LCD", "VADC", "SDIO", "SSP0", "SSP1", "UART0",
    "UART1", "UART2", "UART3", "OUT", "21", "22", "23", "24",
    "OUT0", "OUT1"
};


void clock_report (void)
{
    printf ("Frequencies...\n");
    calibrate();
    printf ("Calibration: %6u kHz\n",
            (16000 * 12000 * 2 + calibration) / (2 * calibration));
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

    for (int i = 0; i != sizeof clock_names / sizeof clock_names[0]; ++i) {
        const char * name = clock_names[i];
        if (name == NULL)
            continue;
        bool first = true;

        for (int j = 0; j != sizeof output_names /sizeof output_names[0]; ++j) {
            unsigned source = * (j + (volatile unsigned *) 0x40050060);
            if (source >> 24 != i || (source & 1))
                continue;

            if (first)
                printf ("%s to", clock_names[i]);
            first = false;

            printf (" %s", output_names[j]);
        }
        if (!first)
            printf ("\n");
    }

    unsigned base = (unsigned) &frequency;
    if (base < 0x10000000) {
        printf ("Using shadow area\n");
        base = *M4MEMMAP;
    }
    const char * upper = "";
    if (base & 0x00040000)
        upper = " (upper)";
    bool b = (base & 0x01000000) != 0;
    base &= 0xfe000000;
    if (base == 0x10000000)
        printf ("Running from RAM\n");
    else if (base == 0x1a000000)
        printf ("Running from flash%c%s\n", 'A' + b, upper);
    else
        printf ("Running from 0x%08x\n", base);
}
