
#include "monkey.h"
#include "registers.h"


void meminit (void)
{
    // Enable CLK_M4_EMC and CLK_M4EMC_DIV.
    * (v32 *) 0x40051430 = 1;
    * (v32 *) 0x40051478 = 1;

    // Setup pins.
    static const unsigned pins[] = {
#define pin_in(a,b,f) (a * 32 * 256 + b * 256 + 0xe0 + f)
#define pin_out(a,b,f) (a * 32 * 256 + b * 256 + 0x20 + f)
        pin_in (1, 7, 3),               // T5  EMC_D0
        pin_in (1, 8, 3),               // R7  EMC_D1
        pin_in (1, 9, 3),               // T7  EMC_D2
        pin_in (1,10, 3),               // R8  EMC_D3
        pin_in (1,11, 3),               // T9  EMC_D4
        pin_in (1,12, 3),               // R9  EMC_D5
        pin_in (1,13, 3),               // R10 EMC_D6
        pin_in (1,14, 3),               // R11 EMC_D7
        pin_in (5, 4, 2),               // P9  EMC_D8
        pin_in (5, 5, 2),               // P10 EMC_D9
        pin_in (5, 6, 2),               // T13 EMC_D10
        pin_in (5, 7, 2),               // R12 EMC_D11
        pin_in (5, 0, 2),               // N3  EMC_D12
        pin_in (5, 1, 2),               // P3  EMC_D13
        pin_in (5, 2, 2),               // R4  EMC_D14
        pin_in (5, 3, 2),               // T8  EMC_D15

        pin_out (2, 9, 3),              // H16 EMC_A0
        pin_out (2,10, 3),              // G16 EMC_A1
        pin_out (2,11, 3),              // F16 EMC_A2
        pin_out (2,12, 3),              // E15 EMC_A3
        pin_out (2,13, 3),              // C16 EMC_A4
        pin_out (1, 0, 2),              // P2  EMC_A5
        pin_out (1, 1, 2),              // R2  EMC_A6
        pin_out (1, 2, 2),              // R3  EMC_A7
        pin_out (2, 8, 3),              // J16 EMC_A8
        pin_out (2, 7, 3),              // H14 EMC_A9
        pin_out (2, 6, 2),              // K16 EMC_A10
        pin_out (2, 2, 2),              // M15 EMC_A11
        pin_out (2, 1, 2),              // N15 EMC_A12
        pin_out (2, 0, 2),              // T16 EMC_A13
        pin_out (6, 8, 1),              // H13 EMC_A14

        pin_out (6,12, 3),              // G15 EMC_DQMOUT0
        pin_out (6,10, 3),              // H15 EMC_DQMOUT1

        pin_out (6, 5, 3),              // P16 EMC_RAS
        pin_out (6, 4, 3),              // R16 EMC_CAS

        pin_out (13, 1, 2),             // P1 EMC_CKEOUT2
        pin_out (13,14, 2),             // R13 EMC_DYCS2

        pin_out (1, 6, 3),              // T4 EMC_WE
    };

    for (int i = 0; i != sizeof pins / sizeof pins[0]; ++i) {
        unsigned pin = pins[i] >> 8;
        unsigned config = pins[i] & 0xff;
        SFSP[0][pin] = config;
        //SFSP[pin >> 5][pin & 31] = config;
    }

    // The datasheet says all clocks and allow input.
    SFSCLK[0] = 0xe0;                   // N5, function 0
    SFSCLK[1] = 0xe0;                   // T10, function 0
    SFSCLK[2] = 0xe5;                   // D14 EMC_CLK23, CLK2 func 5
    SFSCLK[3] = 0xe0;                   // P12, function 0 (?)

    // The -7e device is CL3 at 143MHz, CL2 at 133MHz.

    *DYNAMICRASCAS2 = 0x0202;           // RAS, CAS latencies.
    *DYNAMICREADCONFIG = 1;

    // EMCDELAYCLK?

#define NS2CLK(x) ((x) * 96 / 1000)
    // Settings for 96MHz.
    *DYNAMICRP = NS2CLK(15);            // Precharge period, tRP = 15ns.
    *DYNAMICRAS = NS2CLK(37);           // Active to precharge, tRAS = 37ns.
    *DYNAMICSREX = NS2CLK(67);          // tSREX or tXSR=67ns
    *DYNAMICAPR = 3;                    // tAPR. ???  Using tDAL.
    *DYNAMICDAL = 3;                    // tDAL=4clocks or tAPW.
    *DYNAMICWR = NS2CLK(14);            // Write recovery, tWR=14ns.
    *DYNAMICRC = NS2CLK(60);            // Active to active, tRC=60ns.
    *DYNAMICRFC = NS2CLK(66);           // tRFC, auto refresh period = 66ns.
    *DYNAMICXSR = NS2CLK(67);           // Self-refresh exit time, tXSR=67ns.
    // The datasheet says tRRD = 14tCK but I think they mean 14ns.
    *DYNAMICRRD = NS2CLK(14);           // Bank-to-bank time, tRRD = 14tCK!!!.
    *DYNAMICMRD = 1;                    // Load mode to active, tMRD = 2clock.

    *DYNAMICCONFIG2 = 0x0680;           // Row, bank, column, 16M x 16.

    for (int i = 0; i != 10000; ++i)
        asm volatile ("");

    *DYNAMICCONTROL = 0x0183;           // Issue NOP.

    for (int i = 0; i != 20000; ++i)
        asm volatile ("");

    *DYNAMICCONTROL = 0x0103;           // Issue precharge-all.

    *DYNAMICREFRESH = 5;                // 5*16 = 80 cycles.

    // Perform at least 2 refresh cycles at around 80ns.  This is heaps.
    for (int i = 0; i != 1000; ++i)
        asm volatile ("");

    // 64ms @ 96MHz = 6144000 cycles.   For 8192 rows gives 750 cycles / row.
    // The unit for the register is 16 cycles.
    *DYNAMICREFRESH = 96 * 64000 / 8192 / 16; // The real refresh period.

    *DYNAMICCONTROL = 0x0083;           // Mode command.

    // Burst mode set-up value: 0x023
    // Column length = 9
    // Width = 1
    // Bank bits = 2
    // Sequential v interleaved probably doesn't matter: 0=sequential.
    * (volatile unsigned char *) (0x60000000 + (0x023 << 12));

    // Normal mode.
    *DYNAMICCONTROL = 0x0000;

    // Enable buffers...
    *DYNAMICCONFIG2 = 0x00080680;
}



void memtest (void)
{
    puts ("Meminit\r\n");
    meminit();

    const unsigned size = 8 << 20;
    volatile unsigned * const sdram = (v32 *) 0x60000000;

    puts ("Memtest: Write\n");
    for (int i = 0; i != size; ++i)
        sdram[i] = i * 0x02030401;

    puts ("Memtest: Read\n");
    for (int i = 0; i != size; ++i) {
        unsigned e = i * 0x02030401;
        unsigned v = sdram[i];
        if (v != e)
            printf ("Memtest: Bugger @ %06x export %08x got %08x\n",
                    i, e, v);
    }
    puts ("Memtest: Done\n");
}
