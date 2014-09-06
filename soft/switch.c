#include "monkey.h"
#include "registers.h"
#include "switch.h"

// data is big endian, lsb align.
static unsigned spi_io (unsigned data, int num_bytes)
{
    // Wait for idle.
    while (SSP0->sr & 16);
    // Clear input FIFO.
    while (SSP0->sr & 4)
        SSP0->dr;

    // Take the SSEL GPIO low.
    GPIO_BYTE[7][16] = 0;

    for (int i = 0; i < num_bytes; ++i)
        SSP0->dr = (data >> (num_bytes - i - 1) * 8) & 255;

    // Wait for idle.
    while (SSP0->sr & 16);

    // Take the SSEL GPIO high again.
    GPIO_BYTE[7][16] = 1;

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


static void mdio_report_word(const char * const * desc, unsigned word)
{
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
            status = " off";
        }
        else if (*p == '-') {
            if (!bit)
                continue;
            ++p;
            status = " disabled";
        }
        else if (!bit)
            continue;
        if (*p)
            printf(" %s%s", p, status);
        else
            printf(" Bit %i%s", i, status);
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
        "0AN", "Force-100", "Loopback", "Reset" };

    printf("Port %i\nControl   :", port);
    mdio_report_word(desc0, miim_get(port, 0));

    static const char * const desc4[16] = {
        "0", "", "", "",
        "", "10-Half", "10-Full", "100-Half",
        "100-Full", "", "Pause", "",
        "", "Remote-Fault", "LP-Ack", "Next-Page" };

    printf("\nLocal  Adv :");
    mdio_report_word(desc4, miim_get(port, 4));
    printf("\nRemote Adv :");
    mdio_report_word(desc4, miim_get(port, 5));

    static const char * const desc1f[16] = {
        "", "Remote Loopback", "Power Save", "Force Link",
        "MDI-X", "Polarity Reverse", "", "",
        ".", ".", ".", "",
        "", "", "", "",
    };
    static const char * const mode[8] = {
        "Reserved", "Auto-Neg", "10-Half", "100-Half",
        "Reserved", "10-Full", "100-Full", "Isolate"
    };

    unsigned stat = miim_get(port, 0x1f);
    printf("\nControl/Status: %s", mode[(stat >> 8) & 7]);
    mdio_report_word(desc1f, stat);
    putchar('\n');
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
    // SMRXD0 - ENET_RXD0 - T12 - P1_15
    // SMRXD1 - ENET_RXD1 - L3 - P0_0
    SFSP[0][0] |= 24; // Disable pull-up, enable pull-down.
    SFSP[1][15] |= 24;

    // Switch reset is E16, GPIO7[9], PE_9.
    GPIO_BYTE[7][9] = 0;
    GPIO_DIR[7] |= 1 << 9;
    SFSP[14][9] = 4;                    // GPIO is function 4....
    // Out of reset...
    GPIO_BYTE[7][9] = 1;

    // Wait ~ 100us.
    for (int i = 0; i < 160000; ++i)
        asm volatile ("");

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
    SSP0->cpsr = 2;

    // Keep clock HI while idle, CPOL=1,CPHA=1
    // Output data on falling edge.  Read data on rising edge.
    // Divide clock by 30.
    SSP0->cr0 = 0x00001dc7;

    // Enable SSP0.
    SSP0->cr1 = 0x00000002;

    // Set up the pins.
    SFSP[3][3] = 2; // Clock pin, has high drive but we don't want that.
    SFSP[3][7] = 5;
    SFSP[3][6] = 0x45; // Function 5, enable input buffer.

    // Write registers 0x7c, 0x7d to clear and enable interrupt generation.
    spi_io(0x027c1f1f, 4);

    spi_reg_write (1, 1);               // Start switch.

    puts ("Switch is running\n");
}
