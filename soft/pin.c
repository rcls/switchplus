#include "monkey.h"
#include "pin.h"
#include "registers.h"

void config_pins(const unsigned * pins, int count)
{
    volatile unsigned * sfsp = (volatile unsigned *) 0x40086000;
    for (int i = 0; i < count; ++i) {
        unsigned pin = pins[i] >> 16;
        unsigned config = pins[i] & 0xffff;
        sfsp[pin] = config;
    }
}


static void enter_dfu_go(void)
{
    NVIC_ICER[0] = 0xffffffff;
    NVIC_ICER[1] = 0xffffffff;

    // Switch back to IDIVC.
    *BASE_M4_CLK = 0x0e000800;

    // Turn on the red LED D13, ball H5, P8_1, GPIO4[1].
    GPIO_BYTE[4][1] = 1;
    GPIO_DIR[4] |= 1 << 1;
    SFSP[8][1] = 0;

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
        for (int i = 0; !monkey_is_empty() && i != 1000000; ++i);

    enter_dfu_go();
}


void check_for_early_dfu(void)
{
    GPIO_DIR[5] &= ~0x10;
    GPIO_DIR[6] &= ~0x3000;

    static const unsigned pins[] = {
        PIN_IN(12,14,4),                // TCK is GPIO6[13] PC_14 func 4 ball N1
        PIN_IN(12,13,4),                // TDI is GPIO6[12] PC_13 func 4 ball M1
        PIN_IN(2,4,4),                  // TMS is GPIO5[4] P2_4 func 4 ball K11
    };

    config_pins(pins, sizeof pins / sizeof pins[0]);

    for (int i = 0; i != 10000; ++i)
        asm volatile("");

    if (GPIO_BYTE[6][13] && GPIO_BYTE[6][12] && GPIO_BYTE[5][4])
        return;

    // Reset the switch.  Switch reset is E16, GPIO7[9], PE_9.
    GPIO_BYTE[7][9] = 0;
    GPIO_DIR[7] |= 1 << 9;
    SFSP[14][9] = 4;                    // GPIO7[9], function 4.

    for (int i = 0; i != 10000; ++i)
        asm volatile("");

    GPIO_BYTE[7][9] = 1;

    // 50 MHz in from eth_tx_clk
    // Configure the clock to USB.
    // Generate 480MHz off 50MHz...
    // ndec=5, mdec=32682, pdec=0
    // selr=0, seli=28, selp=13
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    *PLL0USB_CTRL = 0x03000819;
    *PLL0USB_MDIV = (28 << 22) + (13 << 17) + 32682;
    *PLL0USB_NP_DIV = 5 << 12;
    *PLL0USB_CTRL = 0x03000818;         // Divided in, direct out.

    // Wait for locks.
    while (!(*PLL0USB_STAT & 1));

    enter_dfu_go();
}
