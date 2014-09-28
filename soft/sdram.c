
#include "configure.h"
#include "freq.h"
#include "monkey.h"
#include "registers.h"
#include "sdram.h"

void meminit (unsigned mhz)
{
    verbose("SDRAM init @ %d MHz\n", mhz);

    // Setup pins.
    static const unsigned pins[] = {
        BIT_SET(RESET_CTRL[0], 21),

        PIN_IO_FAST(1, 7, 3),           // T5  EMC_D0
        PIN_IO_FAST(1, 8, 3),           // R7  EMC_D1
        PIN_IO_FAST(1, 9, 3),           // T7  EMC_D2
        PIN_IO_FAST(1,10, 3),           // R8  EMC_D3
        PIN_IO_FAST(1,11, 3),           // T9  EMC_D4
        PIN_IO_FAST(1,12, 3),           // R9  EMC_D5
        PIN_IO_FAST(1,13, 3),           // R10 EMC_D6
        PIN_IO_FAST(1,14, 3),           // R11 EMC_D7
        PIN_IO_FAST(5, 4, 2),           // P9  EMC_D8
        PIN_IO_FAST(5, 5, 2),           // P10 EMC_D9
        PIN_IO_FAST(5, 6, 2),           // T13 EMC_D10
        PIN_IO_FAST(5, 7, 2),           // R12 EMC_D11
        PIN_IO_FAST(5, 0, 2),           // N3  EMC_D12
        PIN_IO_FAST(5, 1, 2),           // P3  EMC_D13
        PIN_IO_FAST(5, 2, 2),           // R4  EMC_D14
        PIN_IO_FAST(5, 3, 2),           // T8  EMC_D15

        PIN_OUT_FAST(2, 9, 3),          // H16 EMC_A0
        PIN_OUT_FAST(2,10, 3),          // G16 EMC_A1
        PIN_OUT_FAST(2,11, 3),          // F16 EMC_A2
        PIN_OUT_FAST(2,12, 3),          // E15 EMC_A3
        PIN_OUT_FAST(2,13, 3),          // C16 EMC_A4
        PIN_OUT_FAST(1, 0, 2),          // P2  EMC_A5
        PIN_OUT_FAST(1, 1, 2),          // R2  EMC_A6
        PIN_OUT_FAST(1, 2, 2),          // R3  EMC_A7
        PIN_OUT_FAST(2, 8, 3),          // J16 EMC_A8
        PIN_OUT_FAST(2, 7, 3),          // H14 EMC_A9
        PIN_OUT_FAST(2, 6, 2),          // K16 EMC_A10
        PIN_OUT_FAST(2, 2, 2),          // M15 EMC_A11
        PIN_OUT_FAST(2, 1, 2),          // N15 EMC_A12
        PIN_OUT_FAST(2, 0, 2),          // T16 EMC_A13
        PIN_OUT_FAST(6, 8, 1),          // H13 EMC_A14

        PIN_OUT_FAST(6,12, 3),          // G15 EMC_DQMOUT0
        PIN_OUT_FAST(6,10, 3),          // H15 EMC_DQMOUT1

        PIN_OUT_FAST(6, 5, 3),              // P16 EMC_RAS
        PIN_OUT_FAST(6, 4, 3),              // R16 EMC_CAS

        PIN_OUT_FAST(13, 1, 2),             // P1 EMC_CKEOUT2
        PIN_OUT_FAST(13,14, 2),             // R13 EMC_DYCS2

        PIN_OUT_FAST(1, 6, 3),              // T4 EMC_WE

        WORD_WRITE(SFSCLK[0], 0xe0),    // N5, function 0
        WORD_WRITE(SFSCLK[1], 0xe0),    // T10, function 0
        WORD_WRITE(SFSCLK[2], 0xe5),    // D14 EMC_CLK23, CLK2 func 5
        WORD_WRITE(SFSCLK[3], 0),
    };

    configure(pins, sizeof pins / sizeof pins[0]);

    // Delays...
    if (mhz > 121)
        *EMCDELAYCLK = 0x2222;
    else if (mhz > 97)
        *EMCDELAYCLK = 0x1111;
    else
        *EMCDELAYCLK = 0;

    // Make sure reset is complete.
    while (!(RESET_ACTIVE_STATUS[0] & (1 << 21)));

    // The -7e device is CL3 at 143MHz, CL2 at 133MHz.

    if (mhz >= 143)
        EMC->port[2].rascas = 0x0303;   // RAS, CAS latencies.
    else
        EMC->port[2].rascas = 0x0202;   // RAS, CAS latencies.

    EMC->read_config = 1;

#define NS2CLK(x) ((x) * mhz / 1000)
    // Settings for 96MHz.
    EMC->rp = NS2CLK(15);            // Precharge period, tRP = 15ns.
    EMC->ras = NS2CLK(37);           // Active to precharge, tRAS = 37ns.
    EMC->srex = NS2CLK(67);          // tSREX or tXSR=67ns
    EMC->apr = NS2CLK(15+14);        // tAPR. ???  Using tDAL=tWR+tRP.
    if (mhz > 97)
        EMC->dal = 4;                // Seems good to 160MHz.
    else
        EMC->dal = 3;
    EMC->wr = NS2CLK(14);            // Write recovery, tWR=14ns.
    EMC->rc = NS2CLK(60);            // Active to active, tRC=60ns.
    EMC->rfc = NS2CLK(66);           // tRFC, auto refresh period = 66ns.
    EMC->xsr = NS2CLK(67);           // Self-refresh exit time, tXSR=67ns.
    // The datasheet says tRRD = 14tCK but I think they mean 14ns.
    EMC->rrd = NS2CLK(14);           // Bank-to-bank time, tRRD = 14tCK!!!.
    EMC->mrd = 1;                    // Load mode to active, tMRD = 2clock.

    EMC->port[2].config = 0x0680;       // Row, bank, column, 16M x 16.

    spin_for(100 * mhz);

    EMC->dynamic_control = 0x0183;      // Issue NOP.

    spin_for(200 * mhz);

    EMC->dynamic_control = 0x0103;      // Issue precharge-all.

    EMC->refresh = 1 + NS2CLK(5);    // 5*16 = 80 ns.

    // Perform at least 2 refresh cycles at around 80ns.  This is heaps.
    spin_for(mhz);

    // 64ms @ 96MHz = 6144000 cycles.   For 8192 rows gives 750 cycles / row.
    // The unit for the register is 16 cycles.
    EMC->refresh = mhz * 64000 / 8192 / 16; // The real refresh period.

    EMC->dynamic_control = 0x0083;           // Mode command.

    // Burst mode set-up value: 0x023
    // Column length = 9
    // Width = 1
    // Bank bits = 2
    // Sequential v interleaved probably doesn't matter: 0=sequential.
    if (mhz >= 143)
        * (volatile unsigned char *) (0x60000000 + (0x033 << 12));
    else
        * (volatile unsigned char *) (0x60000000 + (0x023 << 12));

    // Normal mode.
    EMC->dynamic_control = 0x0000;

    // Enable buffers...
    EMC->port[2].config = 0x00080680;
}


int memtest1 (unsigned bit, unsigned w0, unsigned w1)
{
    if (w0 == w1 || bit == 0)
        printf ("Memtest: pattern %08x", w0);
    else if (w0 != ~w1)
        printf ("Memtest: address & %x ? %08x : %08x", bit, w1, w0);
    else if (w0 == 0)
        printf ("Memtest: address & %x", bit);
    else if (w1 == 0)
        printf ("Memtest: address & %x inverted", bit);
    else
        printf ("Memtest: address & %x flip %08x", bit, w0);

    const unsigned size = 8 << 20;
    volatile unsigned * const sdram = (v32 *) 0x60000000;

    for (int i = 0; i < size; ++i)
        sdram[i] = i & bit ? w1 : w0;
    putchar ('\n');
    for (int i = 0; i < size; ++i) {
        unsigned v = sdram[i];
        if (v != (i & bit ? w1 : w0)) {
            printf ("Memtest: Bugger @ %06x expect %08x got %08x\n",
                    i, i & bit ? w1 : w0, v);
            if (peekchar_nb() >= 0)
                return 0;
        }
    }
    return peekchar_nb();
}


void memtest (void)
{
    puts ("Memtest: Init\n");

    //unsigned base_m4 = *BASE_M4_CLK >> 24;
    //unsigned base_m4 = *((v32 *) 0x4005006c) >> 24;

    const unsigned size = 8 << 20;
    volatile unsigned * const sdram = (v32 *) 0x60000000;

    puts ("Memtest: basic.");
    for (unsigned i = 0; i != size; ++i)
        sdram[i] = i * 0x02030401;

    putchar ('\n');
    for (int i = 0; i != size; ++i) {
        unsigned e = i * 0x02030401;
        unsigned v = sdram[i];
        if (v != e) {
            printf ("Memtest: Bugger @ %06x expect %08x got %08x\n", i, e, v);
            if (peekchar_nb() >= 0)
                break;
        }
    }

    if (peekchar_nb() >= 0)
        return;

    static unsigned patterns[] = {
        0xaaaaaaaa, 0x55555555, 0xcccccccc, 0x33333333, 0xf0f0f0f0, 0x0f0f0f0f,
        0xff00ff00, 0x00ff00ff, 0xffff0000, 0x0000ffff,
    };
    for (int j = 0; j != sizeof patterns / sizeof patterns[0]; ++j)
        if (memtest1 (0, patterns[j], patterns[j]) >= 0)
            return;

    for (unsigned bit = 1; bit < 65536; bit <<= 1) {
        if (memtest1 (0, bit | 0xffff0000, bit | 0xffff0000) >= 0)
            return;
        if (memtest1 (0, ~bit & 0xffff, ~bit & 0xffff) >= 0)
            return;
        if (memtest1 (0, (bit * 65537) ^ 0xffff, (bit * 65537) ^ 0xffff) >= 0)
            return;
    }

    for (unsigned bit = 1; bit < size; bit <<= 1) {
        if (memtest1 (bit, 0, 0xffffffff) >= 0)
            return;
        if (memtest1 (bit, 0xffffffff, 0) >= 0)
            return;
    }

    puts ("Memtest: Done\n");
}
