#include "configure.h"
#include "monkey.h"
#include "registers.h"

// Most stuff in this file can get run before globals are initialised (because
// of check-for-early-DFU).

void configure(const unsigned * pins, int count)
{
    for (int i = 0; i < count; ++i) {
        unsigned numberL = pins[i] & 0xfffffff;
        volatile unsigned * addressL = (void *) 0x40000000 + numberL;
        volatile unsigned * addressS = (void *) 0x40000000 + (pins[i] & 0xfffff);
        unsigned opcode = pins[i] >> 28;
        switch (opcode) {
        case op_zero: case op_one:      // Write zero/one.
            *addressL = opcode;
            break;
        case op_write32: {              // Write words.
            unsigned count = numberL >> 20;
            for (unsigned j = 0; j <= count; ++j)
                addressS[j] = pins[++i];
            break;
        }
        case op_spin:
            spin_for(numberL);
            break;
        case op_wait_zero:
            while (*addressL);
            break;
        default:                        // Write word small.
            for (unsigned i = op_write; i <= opcode; ++i)
                *addressS++ = numberL >> 20;
            break;
        }
    }
}


static void enter_dfu_go(void)
{
    NVIC_ICER[0] = 0xffffffff;
    NVIC_ICER[1] = 0xffffffff;

    static const unsigned pins[] = {
        // Switch back to IDIVC.
        WORD_WRITE32(*BASE_M4_CLK, 0x0e000800),

        // Turn on the green LED D12, ball K4, P8_2, GPIO4[2].
        WORD_WRITE(GPIO_WORD[4][2], 1),
        BIT_SET(GPIO_DIR[4], 2),
        PIN_OUT(8, 2, 0),

        WORD_WRITE(*USBCMD, 2),         // Reset USB.
        BIT_WAIT_ZERO(*USBCMD, 1),
    };
    configure(pins, sizeof pins / sizeof pins[0]);

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

        // 50 MHz in from eth_tx_clk
        // Configure the clock to USB.
        // Generate 480MHz off 50MHz...
        // ndec=5, mdec=32682, pdec=0
        // selr=0, seli=28, selp=13
        // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
        WORD_WRITE32n(PLL0USB->ctrl, 3,
                      0x30000819,        // CTRL : Divided in, direct out.
                      (28 << 22) + (13 << 17) + 32682, // MDIV.
                      5 << 12),                        // NP_DIV

        BIT_RESET(PLL0USB->ctrl, 0),

        // Wait for lock.
        BIT_WAIT_ZERO(PLL0USB->stat, 0),
    };
    configure(swreset, sizeof swreset / sizeof swreset[0]);

    enter_dfu_go();
}
