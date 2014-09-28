#include "configure.h"
#include "monkey.h"
#include "registers.h"
#include "switch.h"


// Data is big endian, LSB aligned.
static unsigned spi_io (unsigned data, int num_bytes)
{
    // Wait for idle.
    while (SSP0->sr & 16);
    // Clear input FIFO.
    while (SSP0->sr & 4)
        SSP0->dr;

    GPIO_BYTE[7][16] = 0;               // Take the SSEL GPIO low.

    for (int i = 0; i < num_bytes; ++i)
        SSP0->dr = (data >> (num_bytes - i - 1) * 8) & 255;

    // Wait for idle.
    while (SSP0->sr & 16);

    GPIO_BYTE[7][16] = 1;               // Take the SSEL GPIO high again.

    unsigned result = 0;
    for (int i = 0; i < num_bytes; ++i) {
        while (!(SSP0->sr & 4));
        result = result * 256 + (SSP0->dr & 255);
    }

    return result;
}


static void spi_reg_write (unsigned address, unsigned data)
{
    spi_io (0x20000 + (address & 255) * 256 + data, 3);
}


static unsigned spi_reg_read (unsigned address)
{
    return spi_io (0x30000 + (address & 255) * 256, 3) & 255;
}


static unsigned miim_get(int port, int item)
{
    MAC->mii_addr = (port << 11) + (item << 6) + (5 << 2) + 1;
    while (MAC->mii_addr & 1);          // Poll for result.
    return MAC->mii_data;
}


static void mdio_report_word(const char * tag,
                             const char * const * desc, unsigned word)
{
    printf("\n  %s:", tag);
    for (int i = 0; i != 16; ++i, word >>= 1) {
        bool bit = word & 1;
        const char * p = desc[i];
        if (*p == '~') {
            bit = !bit;
            ++p;
        }
        const char * status = "";
        if (*p == '.')
            continue;
        if (*p == '0') {
            if (bit)
                continue;
            ++p;
            status = "-off";
        }
        else if (*p == '-') {
            if (!bit)
                continue;
            ++p;
            status = "-disabled";
        }
        else if (!bit)
            continue;
        if (*p)
            printf(" %s%s", p, status);
        else
            printf(" Bit-%i%s", i, status);
    }
}


void mdio_report_port(int port)
{
    // Leading characters: ~ : invert bit", + : report 1 only, - : report
    // 1 as disabled only. 0 : report 0 as off only, ".": ignore.
    static const char * const desc0[16] = {
        "-LED", "-TX", "-Fault detect", "-Auto-MDI",
        "Force-MDI", "~+Micrel-MDI", "", "Collision-Test",
        "Force-FD", "Restart-AN", "Isolate", "Power-Down",
        "0AutoNeg", "Force-100", "Loopback", "Reset" };

    printf("Port %i", port);
    mdio_report_word("Control    ", desc0, miim_get(port, 0));

    static const char * const desc1[16] = {
        "+Ext.-Capable", "+Jabber-Test", "Link-Up", "0Auto-Neg",
        "Fault", "Auto-Neg-Done", "1Preamble", "",
        "", "", "", "010-Half",
        "010-Full", "0100-Half", "0100-Full", "T4-Capable"
    };
    mdio_report_word("Status     ", desc1, miim_get(port, 1));

    static const char * const desc4[16] = {
        "0", "", "", "",
        "", "10-Half", "10-Full", "100-Half",
        "100-Full", "", "Pause", "",
        "", "Remote-Fault", "LP-Ack", "Next-Page" };

    mdio_report_word("Local  Adv ", desc4, miim_get(port, 4));
    mdio_report_word("Remote Adv ", desc4, miim_get(port, 5));

    static const char * const desc1f[16] = {
        "", "Remote-Loopback", "Power-Save", "Force-Link",
        "MDI-X", "Polarity-Reverse", "", "",
        ".", ".", ".", "",
        "", "", "", "",
    };
    static const char * const mode[8] = {
        "Reserved", "Auto-Neg", "10-Half", "100-Half",
        "Reserved", "10-Full", "100-Full", "Isolate"
    };

    unsigned stat = miim_get(port, 0x1f);
    mdio_report_word("Special    ", desc1f, stat);
    printf(" ** %s **\n", mode[(stat >> 8) & 7]);
}


void mdio_report_all (void)
{
    for (int i = 1; i <= 5; ++i)
        mdio_report_port(i);

    /* int isr = spi_reg_read(124) & 0xff; */
    /* printf("Interrupt status: %02x\n", isr); */
    /* printf("Interrupt enable: %02x\n", spi_reg_read(125) & 0xff); */
    /* spi_reg_write(124, isr); */
}


void mdio_report_changed (void)
{
    int status = spi_reg_read(124);
    spi_reg_write(124, status);
    for (int i = 0; i < 5; ++i)
        if (status & (1 << i))
            mdio_report_port(i + 1);
}


void init_switch (void)
{
    // Configure SSP0...
    *BASE_SSP0_CLK = 0x03000800;        // Base clock is 50MHz.

    // Set up the pins.
#define PULL_DOWN 0x18
    static const unsigned pins[] = {
        // Set the prescaler to divide by 8.
        WORD_WRITE(SSP0->cpsr, 8),

        // Keep clock HI while idle, CPOL=1,CPHA=1
        // Output data on falling edge.  Read data on rising edge.
        // No clock divide (6.25MHz).
        WORD_WRITE(SSP0->cr0, 0xc7),

        // Enable SSP0.
        WORD_WRITE(SSP0->cr1, 2),

        // Set SPIS output hi.
        BYTE_ONE(GPIO_BYTE[7][16]),
        BIT_SET(GPIO_DIR[7], 16),

        // Switch reset is E16, GPIO7[9], PE_9.
        BYTE_ZERO(GPIO_BYTE[7][9]),
        BIT_SET(GPIO_DIR[7], 9),

        PIN_OUT(14,9,4),                // Switch reset, GPIO7[9], function 4.

        PIN_OUT(15,1,4),                // SPIS, E11, GPIO7[16], function 4.
        PIN_OUT(3,3,2),                 // SPIC, B14, SSP0 Clock.
        PIN_OUT(3,7,5),                 // SPID, C11, SSP0 MOSI.
        PIN_IN(3,6,5),                  // SPIQ, B13, SSP0 MISO.

        PIN_IO(1,17,3),                 // MDIO (M8, P1_17, func 3)
        PIN_OUT(12,1,3),                // MDC (E4, PC_1, func 3).

        PIN_IN_FAST(1,19,0),            // TX_CLK (M11, P1_19, func 0)
        PIN_IN_FAST(1,15,PULL_DOWN | 3), // RXD0 (T12, P1_15, func 3)
        PIN_IN_FAST(0,0,PULL_DOWN | 2), // RXD1 (L3, P0_0, func 2)
        PIN_IN_FAST(12,8,3),            // RX_DV (N4, PC_8, func 3)

        PIN_OUT(1,18,3),                // TXD0, N12, P1_18, func 3.
        PIN_OUT(1,20,3),                // TXD1, M10, P1_20, func 3.
        PIN_OUT(0,1,6),                 // TX_EN, M2, P0_1, func 6.

        // Out of reset.
        BYTE_ONE(GPIO_BYTE[7][9]),

        // Wait milliseconds (docs say ~ 100us).
        SPIN_FOR(96000),
    };
    configure(pins, sizeof pins / sizeof pins[0]);

    // Take SPI to high speed.
    //spi_reg_write(12, 0x64);

    // Write registers 0x7c, 0x7d to clear and enable interrupt generation.
    spi_io(0x027c1f1f, 4);

    spi_reg_write (1, 1);               // Start switch.

    puts ("Switch is running\n");
}
