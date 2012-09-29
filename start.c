// Let's try and start the switch.

// Pull down:
// [wait 10ms]
// 82 SMRXD1 IPD - port 5 100 Mbps - PULL DOWN
// 83 SMRXD0 IPD - leds - PULL DOWN
// and reset...
// wait 50ns (at 12MHz, one clock cycle...
// pull reset high.
// wait 50ns
// ... reset GPIOs ...

// Bring up SPI interface...

#include "registers.h"

// data is big endian, lsb align.
static unsigned spi_io (unsigned data, int num_bytes)
{
    // Wait for idle.
    while (*SSP0_SR & 16);
    // Clear input FIFO.
    while (*SSP0_SR & 4)
        *SSP0_DR;

    // Take the SSEL GPIO low.
    GPIO_BYTE[7][16] = 0;

    for (int i = 0; i < num_bytes; ++i)
        *SSP0_DR = (data >> (num_bytes - i - 1) * 8) & 255;

    // Wait for idle.
    while (*SSP0_SR & 16);

    // Take the SSEL GPIO high again.
    GPIO_BYTE[7][16] = 1;

    unsigned result = 0;
    for (int i = 0; i < num_bytes; ++i) {
        while (!(*SSP0_SR & 4));
        result = result * 256 + (*SSP0_DR & 255);
    }

    return result;
}

static unsigned spi_reg_read (unsigned address)
{
    return spi_io (0x30000 + ((address & 255) * 256), 3) & 255;
}

static unsigned spi_reg_write (unsigned address, unsigned data)
{
    return spi_io (0x20000 + ((address & 255) * 256) + data, 3) & 255;
}

static void ser_w_byte (unsigned byte)
{
#if 0
    while (!(*UART3_LSR & 32));         // Wait for THR to be empty.
    *UART3_THR = byte;
#endif
}

static void ser_w_string (const char * s)
{
    for (; *s; s++)
        ser_w_byte (*s);
}

static void ser_w_hex (unsigned value, int nibbles, const char * term)
{
    for (int i = nibbles; i != 0; ) {
        --i;
        ser_w_byte("0123456789abcdef"[(value >> (i * 4)) & 15]);
    }
    ser_w_string (term);
}

//void _start (void) __attribute__ ((section (".start")));
void doit (void)
{
    ser_w_byte ('R');

    // SMRXD0 - ENET_RXD0 - T12 - P1_15
    // SMRXD1 - ENET_RXD1 - L3 - P0_0
    SFSP[0][0] |= 24; // Disable pull-up, enable pull-down.
    SFSP[1][15] |= 25;

    // Switch reset is E16, GPIO7[9], PE_9.
    GPIO_BYTE[7][9] = 0;
    GPIO_DIR[7] |= 1 << 9;
    SFSP[14][9] = 4;                    // GPIO is function 4....
    // Out of reset...
    GPIO_BYTE[7][9] = 1;

    // Wait ~ 100us.
    for (volatile int i = 0; i < 10000; ++i);

    // Switch SPI is on SSP0.
    // SPIS is SSP0_SSEL on E11, PF_1 - use as GPIO7[16], function 4.
    // SPIC is SSP0_SCK on B14, P3_3, function 2.
    // SPID is SSP0_MOSI on C11, P3_7, function 5.
    // SPIQ is SSP0_MISO on B13, P3_6, function 5.

    // Set SPIS output hi.
    GPIO_BYTE[7][16] = 1;
    GPIO_DIR[7] |= 1 << 16;
    SFSP[15][1] = 4;                    // GPIO is function 4.

    // Set the prescaler to divide by 2.
    *SSP0_CPSR = 2;

    // Is SSP0 unit clock running by default?  "All branch clocks are enabled
    // by default".
    // Keep clock HI while idle, CPOL=1,CPHA=1
    // Output data on falling edge.  Read data on rising edge.
    // Divide clock by 30.
    *SSP0_CR0 = 0x00001dc7;

    // Enable SSP0.
    *SSP0_CR1 = 0x00000002;

    // Set up the pins.
    SFSP[3][3] = 2; // Clock pin, has high drive but we don't want that.
    SFSP[3][7] = 5;
    SFSP[3][6] = 0x45; // Function 5, enable input buffer.

    ser_w_byte ('S');

    /* ser_w_hex (spi_reg_read (0), 2, " reg0  "); */
    /* ser_w_hex (spi_reg_read (1), 2, " reg1 "); */
    spi_reg_write (1, 1);
    /* ser_w_hex (spi_reg_read (1), 2, "\r\n"); */

    while (1) {
        unsigned fakeotp[64];
        for (int i = 0; i != 64; ++i)
            fakeotp[i] = OTP[i];

        fakeotp[12] = 6 << 25;

        typedef unsigned F (void*);
        ((F*) 0x1040158d) (fakeotp);
    }
}

unsigned start[70] __attribute__ ((section (".start")));
unsigned start[70] = {
    0x10089ff0,
    (unsigned) doit
};
