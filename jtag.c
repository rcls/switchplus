
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


static void wait(void)
{
    for (int i = 0; i != 25; ++i)
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


static void jtag_ir(int n, unsigned d)
{
    // Select DR, Select IR, Capture IR, Shift IR.
    jtag_tms (4, 3);
    jtag_tdi (n, d);                    // Exit1 IR.
    jtag_clk (1, 1);                    // Update IR.
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
    log_serial = false;                 // We share pins with the serial log...

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


void jtag_cmd (void)
{
    jtag_reset();
#define CLR "\r\e[K"

    while (true) {
        printf ("<R>eset, <I>d, <D>...");
        switch (getchar()) {
        case 'r':
            printf (CLR "Reset\n");
            jtag_reset();
            break;

        case 'i':
            printf (CLR "Send ID command\n");
            jtag_ir(6,9);
            break;

        case 'b':
            printf (CLR "Send Bypass command\n");
            jtag_ir(6, 0x3f);
            break;

        case 'd':
            printf (CLR "32 DR bits %08x\n", jtag_dr_short(32, 0xdeadbeef));
            break;

        default:
            printf (CLR "Jtag exit...\n");
            return;
        }
    }
}
