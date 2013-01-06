
#include "callback.h"
#include "monkey.h"
#include "registers.h"
#include "usb.h"

#define TCK GPIO_BYTE[6][13]
#define TDO GPIO_BYTE[5][3]
#define TDI GPIO_BYTE[6][12]
#define TMS GPIO_BYTE[5][4]

#define JTAG_EP_IN 0x84
#define JTAG_EP_OUT 0x04


#define IDCODE 9
#define JSTART 12
#define CFG_IN 5
#define JPROGRAM 11
#define BYPASS 63

// CR + ANSI clear line sequence.
#define CLR "\r\e[K"

static void wait(void)
{
    for (int i = 0; i != 50; ++i)
        asm volatile ("");
}


static int jtag_clk(int tms, int tdi)
{
    TDI = !!tdi;
    TMS = !!tms;
    wait();
    TCK = 0;
    wait();
    int r = TDO;
    TCK = 1;
    return r;
}


static void jtag_tms(int n, unsigned d)
{
    for (int i = 0; i != n; ++i)
        jtag_clk(d & (1 << i), 1);
}


static unsigned jtag_tdi(int n, unsigned d)
{
    unsigned r = 0;
    --n;
    for (int i = 0; i != n; ++i)
        r |= jtag_clk(0, d & (1 << i)) << i;

    r |= jtag_clk(1, d & (1 << n)) << n;
    return r;
}


static unsigned jtag_ir(unsigned d)
{
    // Select DR, Select IR, Capture IR, Shift IR.
    jtag_tms (4, 3);
    unsigned r = jtag_tdi (6, d);       // Exit1 IR.
    jtag_clk (1, 1);                    // Update IR.
    return r;
}


static unsigned jtag_dr_short(int n, unsigned d)
{
    jtag_tms (3, 1);
    unsigned r = jtag_tdi (n, d);       // Exit1 DR.
    jtag_clk (1, 1);                    // Update DR.
    return r;
}


static void jtag_reset(void)
{
    // Set the jtag pins to be GPIO.
    // TCK is GPIO6[13] PC_14 func 4 ball N1
    // TDO is GPIO5[3] P2_3 func 4 ball J12
    // TDI is GPIO6[12] PC_13 func 4 ball M1
    // TMS is GPIO5[4] P2_4 func 4 ball K11

    GPIO_DIR[6] |= 0x3000;
    GPIO_DIR[5] = (GPIO_DIR[5] & ~8) | 0x10;

    TCK = 0;
    TDI = 1;
    TMS = 1;

    SFSP[12][14] = 4;
    SFSP[2][3]   = 0x44;
    SFSP[12][13] = 4;
    SFSP[2][4]   = 4;

    // Reset jtag, land in run test/idle.
    jtag_tms(9,0xff);
    printf ("idcode %08x\n", jtag_dr_short (32, 0xdeadbeef));

    // Reset again.
    jtag_tms(9,0xff);
}


int program_nibble (int next, int c, int * count, int final)
{
    if (next < 0 || *count < 0)
        return c;
    jtag_clk(0, next & 8);
    jtag_clk(0, next & 4);
    jtag_clk(0, next & 2);
    jtag_clk(final, next & 1);
    if ((++*count & 4095) == 0)
        verbose(CLR "Processed %u bits", *count * 4);
    return c;
}


void jtag_program (void)
{
    jtag_tms(9,0xff);                   // Reset, Run-test/idle.
    int r = jtag_ir(JPROGRAM);
    verbose ("JPROGRAM IR %02x\n", r);
    jtag_clk(0,0);                      // Run-test idle.
    for (int i = 0; i != 100000; ++i)
        if (jtag_ir(BYPASS) == 0x11)
            break;

    jtag_clk(0,0); // Run-test idle.
    r = jtag_ir(CFG_IN);
    verbose ("CFG_IN IR %02x\n", r);
    //.... DR bitstream...
    jtag_tms(3,1);
    // The bitstream...
    int next = -1;
    int count = 0;
    while (true) {
        int c = getchar();
        if (c == '.')
            break;
        if (c >= '0' && c <= '9')
            next = program_nibble (next, c, &count, 0);
        else if ((c&~32) >= 'A' && (c&~32) <= 'F')
            next = program_nibble (next, c + 9, &count, 0);
        else if (c > ' ') {
            printf ("\nUnknown char %02x, abort program\n", c);
            count = -1;
        }
    }
    if (next < 0 || count < 0) {
        printf (CLR "No data; aborting\n");
        jtag_reset();
        return;
    }

    program_nibble (next, -1, &count, 1);
    jtag_clk (1, 1);                    // Update DR.

    jtag_clk(0,0);                      // Run-test idle.
    r = jtag_ir(JSTART);
    verbose ("\nJSTART IR %02x\n", r);
    jtag_tms(16,0);                     // Run-test idle.
    jtag_tms(3,7);                      // Reset.
    jtag_clk(0,0);                      // Run-test idle.

    printf (CLR "Programmed %u bits\n", count * 4);
}


void jtag_cmd (void)
{
    jtag_reset();

    while (true) {
        verbose ("JTAG: <R>eset, <I>d, <D>, <B>, <P>...");
        switch (getchar()) {
        case 'j':
            printf (CLR "Reset\n");
            jtag_reset();
            break;

        case 'i':
            printf (CLR "Sent ID command, IR returns %02x\n",
                    jtag_ir(IDCODE));
            break;

        case 'b':
            printf (CLR "Reboot fpga\n");
            jtag_tms(9,0xff);           // Reset, Run-test/idle.
            jtag_ir(JPROGRAM);
            jtag_tms(5,0x1f);
            break;

        case 'd':
            printf (CLR "32 DR bits %08x\n", jtag_dr_short(32, 0xdeadbeef));
            break;

        case 'p':
            printf (CLR "Jtag program....\n");
            jtag_program();
            break;

        default:
            printf (CLR "Jtag exit...\n");
            return;
        }
    }
}
