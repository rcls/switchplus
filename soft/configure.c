#include "configure.h"
#include "monkey.h"
#include "registers.h"

// Most stuff in this file can get run before globals are initialised (because
// of check-for-early-DFU).

void configure(const unsigned * pins, int count)
{
    for (int i = 0; i < count; ++i) {
        unsigned address = 0x40000000 + (pins[i] & 0xfffffff);
        unsigned opcode = pins[i] >> 28;
        switch (opcode) {
        case 0: case 1:                 // Write byte.
            * (volatile unsigned char *) address = opcode;
            break;
        case 2:                         // Write word immediate.
            * (volatile unsigned *) address = pins[++i];
            break;
        case 3:
            spin_for(pins[i] & 0xfffffff);
            break;
        default:                        // Write word small.
            * (volatile unsigned *) (address & ~0xff00000)
                = (pins[i] >> 20) - 1024;
            break;
        }
    }
}


static void enter_dfu_go(void)
{
    NVIC_ICER[0] = 0xffffffff;
    NVIC_ICER[1] = 0xffffffff;

    // Switch back to IDIVC.
    *BASE_M4_CLK = 0x0e000800;

    // Turn on the green LED D12, ball K4, P8_2, GPIO4[2].
    GPIO_BYTE[4][2] = 1;
    GPIO_DIR[4] |= 1 << 2;
    SFSP[8][2] = 0;

    *USBCMD = 2;                       // Reset USB.
    while (*USBCMD & 2);

    unsigned fakeotp[64];
    for (int i = 0; i != 64; ++i)
        fakeotp[i] = OTP[i];

    fakeotp[12] = (6 << 25) + (1 << 23); // Boot mode; use custom usb ids.
    fakeotp[13] = 0x524cf055;           // Product id / vendor id.

    typedef unsigned F (void*);
    ((F*) 0x1040158d) (fakeotp);

    while (1);
}


void enter_dfu(void)
{
    bool was_empty = monkey_is_empty();
    verbose ("Enter DFU\n");
    if (was_empty)
        for (int i = 0; !monkey_is_empty() && i != 100000; ++i);

    enter_dfu_go();
}


void check_for_early_dfu(void)
{
    static const unsigned pins[] = {
        BIT_RESET(GPIO_DIR[5], 4),
        BIT_RESET(GPIO_DIR[6], 12),
        BIT_RESET(GPIO_DIR[6], 13),
        PIN_IN(12,14,4),                // TCK is GPIO6[13] PC_14 func 4 ball N1
        PIN_IN(12,13,4),                // TDI is GPIO6[12] PC_13 func 4 ball M1
        PIN_IN(2,4,4),                  // TMS is GPIO5[4] P2_4 func 4 ball K11
        SPIN_FOR(255),                  // Give pull-ups time.
    };

    configure(pins, sizeof pins / sizeof pins[0]);

    if (GPIO_BYTE[6][13] && GPIO_BYTE[6][12] && GPIO_BYTE[5][4])
        return;

    // Reset the switch.  Switch reset is E16, GPIO7[9], PE_9.
    static const unsigned swreset[] = {
        WORD_WRITE(GPIO_WORD[7][9], 0),
        BIT_SET(GPIO_DIR[7], 9),
        PIN_IN_FAST(1,19,0),            // TX_CLK (M11, P1_19, func 0)
        PIN_OUT(14,9,4),                // Switch reset, GPIO7[9], function 4.
        SPIN_FOR(10000),
        WORD_WRITE(GPIO_WORD[7][9], 1),
    };
    configure(swreset, sizeof swreset / sizeof swreset[0]);

    // 50 MHz in from eth_tx_clk
    // Configure the clock to USB.
    // Generate 480MHz off 50MHz...
    // ndec=5, mdec=32682, pdec=0
    // selr=0, seli=28, selp=13
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    PLL0USB->ctrl = 0x03000819;
    PLL0USB->mdiv = (28 << 22) + (13 << 17) + 32682;
    PLL0USB->np_div = 5 << 12;
    PLL0USB->ctrl = 0x03000818;         // Divided in, direct out.

    // Wait for locks.
    while (!(PLL0USB->stat & 1));

    enter_dfu_go();
}
