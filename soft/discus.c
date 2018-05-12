
#include "discus.h"

#include "callback.h"
#include "configure.h"
#include "monkey.h"
#include "registers.h"

#include <stdint.h>

// CPU_CLK is previous LCD_HSYNC.  P7_6 Func 0, GPIO3[14] (C7 -> 16).
// CPU_RESET is what we previously used as PF_10, Func 4, gpio7_24 (A3 -> 12).

const unsigned spi_setup[] __init_script("3") = {
    // Reset ssp1, keep m0 in reset.
    WORD_WRITE32(RESET_CTRL[1], (1 << 19) | (1 << 24)),
    BIT_WAIT_SET(RESET_ACTIVE_STATUS[1], 19),

    WORD_WRITE(SSP1->cpsr, 2),          // Clock pre-scale: 160MHz / 2 = 80MHz.
    // 10-bit xfer, SPI mode zero, clock low between frames, capture on first
    // (rising) edge of frame (we'll output on falling edge).  Divide by 4
    // to give 20MHz.
    WORD_WRITE32(SSP1->cr0, 0x0309),
    WORD_WRITE(SSP1->cr1, 2),           // Enable master.

    PIN_OUT_FAST(15,4,0),               // SCK is D10, PF_4 func 0.
    PIN_OUT_FAST(15,5,2),               // SSEL is E9, PF_5, func 2.
    PIN_IO_FAST (15,6,2),               // MISO is E7, PF_6 func 2.
    PIN_OUT_FAST(15,7,2),               // MOSI is B7, PF_7 func 2.

    PIN_OUT_FAST(7,6,0),                // CPU_CLK is C7, P7_6 func 0.
    PIN_OUT(15,10,4),                   // CPU_RESET is A3, PF_10 func 4.

    BIT_SET(GPIO_DIR[3], 14),
    BIT_SET(GPIO_DIR[7], 24),
};

#define CPU_CLK   (GPIO_BYTE[3][14])
#define CPU_RESET (GPIO_BYTE[7][24])

#define OP_ADDRESS 0
#define OP_PROGRAM 0x100
#define OP_MEMREAD 0x200
#define OP_MEMWRITE 0x300

static uint8_t membuf[256];

static uint8_t spixact(uint32_t val)
{
    // While BSY or RNE set, wait & read the dr register.
    while (SSP1->sr & 0x14)
        SSP1->dr;

    SSP1->dr = val;

    // Wait for RNE.
    while (~SSP1->sr & 4);

    return SSP1->dr;
}


static void spiw(uint32_t val)
{
    // Just wait for TNF.
    while (~SSP1->sr & 2);
    SSP1->dr = val;
}


// Read memory from Discus and report any updates.  We do this in 16 × 16.
// what=-1: print nothing, what=0: print changes, what=+1: print all.
static void memread(int what)
{
    spiw(OP_ADDRESS);

    for (int i = 0; i < 256; i += 16) {
        uint16_t deltamask = 0;
        if (what > 0)
            deltamask = -1;
        for (int j = 0; j < 16; ++j) {
            uint8_t n = spixact(OP_MEMREAD);
            if (n != membuf[i + j])
                deltamask |= 1 << j;

            membuf[i + j] = n;
        }
        if (!deltamask || what < 0)
            continue;
        printf("%02x:", i);
        for (int j = 0; j < 16; ++j) {
            if (deltamask & 1 << j)
                printf(" %02x", membuf[i + j]);
            else
                printf("  .");
        }
        printf("\n");
    }
}


static void discus_write(int op)
{
    // Get the address....
    uint8_t address = 0;
    while (true) {
        int c = getchar();
        if (c == '\n')
            break;
        address = address * 16 + hex_nibble(c);
    }

    spiw(OP_ADDRESS + address);

    printf("\nData: ");

    int v = -1;
    while (true) {
        int c = getchar();
        if (c == '\n')
            break;
        if (v < 0)
            v = hex_nibble(c);
        else {
            spiw(op + v * 16 + hex_nibble(c));
            v = -1;
        }
    }
    if (v >= 0)
        spiw(op + v);
    printf("\n");
}


static void discus_clock(int n)
{
    CPU_RESET = 0;
    for (int i = 0; i < n; ++i) {
        CPU_CLK = 0;
        CPU_CLK = 1;
    }
    CPU_CLK = 0;
    memread(0);
}


static unsigned int get_decimal(void)
{
    unsigned int n = 0;
    while (true) {
        int c = getchar();
        if (c == '\n') {
            printf("\n");
            return n;
        }
        if (c < '0' || c > '9')
            drop_line_restart(CLR "Illegal hex character; aborting...", c);
        putchar(c);
        n = n * 10 + c - '0';
    }
}


void _Noreturn discus_go(void)
{
    while (true) {
        verbose("DISCUS: <p>rogram, mem<w>, <m>emread, <r>eset, <1> clock, <c> n...");
        switch (getchar()) {
        case 'p':
            printf(CLR "Program address: ");
            discus_write(OP_PROGRAM);
            break;
        case 'w':
            printf(CLR "Write address: ");
            discus_write(OP_MEMWRITE);
            break;
        case 'm':
            printf(CLR);
            memread(1);
            break;
        case 'r':
            printf(CLR "Reset\n");
            CPU_RESET = 1;
            for (int i = 0; i < 4; ++i) {
                CPU_CLK = 0;
                CPU_CLK = 1;
            }
            CPU_CLK = 0;
            break;
        case '1':
            printf(CLR "Clock 1\n");
            discus_clock(1);
            break;
        case 'c':
            printf(CLR "Clock: ");
            discus_clock(get_decimal());
            break;
        default:
            printf(CLR "Discus exit...\n");
            start_program(initial_program);
        }
    }
}
