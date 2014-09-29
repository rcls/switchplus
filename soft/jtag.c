
#include "configure.h"
#include "monkey.h"
#include "registers.h"
#include "usb.h"

#define TCK GPIO_WORD[6][13]
#define TDO GPIO_WORD[5][3]
#define TDI GPIO_WORD[6][12]
#define TMS GPIO_WORD[5][4]

#define JTAG_EP_IN 0x84
#define JTAG_EP_OUT 0x04


#define IDCODE 9
#define JSTART 12
#define CFG_IN 5
#define JPROGRAM 11
#define BYPASS 63

// CR + ANSI clear line sequence.
#define CLR "\r\e[K"

static int jtag_clk(int tms, int tdi)
{
    TDI = !!tdi;
    TMS = !!tms;
    spin_for(50);
    TCK = 0;
    spin_for(50);
    int r = TDO;
    TCK = 1;
    return r & 1;
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
    // Set-up the jtag pins to be GPIO.
    static const unsigned pins[] = {
        WORD_WRITE(TCK, 0),
        WORD_WRITE(TDI, 1),
        WORD_WRITE(TMS, 1),

        BIT_SET(GPIO_DIR[6], 12),       // TDI
        BIT_SET(GPIO_DIR[6], 13),       // TCK
        BIT_RESET(GPIO_DIR[5], 3),      // TDO
        BIT_SET(GPIO_DIR[5], 4),        // TMS

        PIN_OUT(12,14,4),               // TCK is GPIO6[13] PC_14 func 4 ball N1
        PIN_IN(2,3,4),                  // TDO is GPIO5[3] P2_3 func 4 ball J12
        PIN_OUT(12,13,4),               // TDI is GPIO6[12] PC_13 func 4 ball M1
        PIN_OUT(2,4,4),                 // TMS is GPIO5[4] P2_4 func 4 ball K11
    };

    configure(pins, sizeof pins / sizeof pins[0]);

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
    printf (CLR "Programmed %u bits\n", count * 4);
    jtag_clk (1, 1);                    // Update DR.

    jtag_clk(0,0);                      // Run-test idle.
    r = jtag_ir(JSTART);
    verbose ("JSTART IR %02x\n", r);
    jtag_tms(16,0);                     // Run-test idle.
    jtag_tms(3,7);                      // Reset.
    jtag_clk(0,0);                      // Run-test idle.
}


void jtag_cmd (void)
{
    jtag_reset();

    while (true) {
        verbose ("JTAG: <I>d, <B>oot, <P>program...");
        switch (getchar()) {
        case 'j':
            printf (CLR "Reset\n");
            jtag_reset();
            break;

        case 'i':
            printf (CLR "Sent ID command, IR returns %02x\n",
                    jtag_ir(IDCODE));
            printf ("32 DR bits %08x\n", jtag_dr_short(32, 0xdeadbeef));
            break;

        case 'b':
            printf (CLR "Reboot fpga\n");
            jtag_tms(9,0xff);           // Reset, Run-test/idle.
            jtag_ir(JPROGRAM);
            jtag_tms(5,0x1f);
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
