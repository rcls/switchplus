
#include "registers.h"

static void ser_w_byte (unsigned byte)
{
    while (!(*UART3_LSR & 32));         // Wait for THR to be empty.
    *UART3_THR = byte;
}

static void ser_w_hex (unsigned value, int nibbles, const char * term)
{
    for (int i = nibbles; i != 0; ) {
        --i;
        ser_w_byte("0123456789abcdef"[(value >> (i * 4)) & 15]);
    }
    for (; *term; ++term)
        ser_w_byte(*term);
}

static void bpc (const char * tag)
{
    ser_w_hex (* (v32 *) 0x40043204, 8, tag);
}

static void mem_copy (unsigned char * dest, const unsigned char * src,
                      unsigned len)
{
    while (len--)
        *dest++ = *src++;
}

void _start (void) __attribute__ ((section (".start")));
void _start (void)
{
    GPIO_BYTE[4][1] = 1;
    GPIO_BYTE[4][2] = 0;
    GPIO_DIR[4] |= 6;
    GPIO_BYTE[4][1] = 0;
    GPIO_BYTE[4][2] = 1;
#if 0
    //Boot modes: P2_9(h16,emc_a0) P2_8(j16,emc_a8) P1_2(r3,emc_a7) P1_1(r2,emc_a6)
    //USB0   low high low high

    // P2_9 = GPIO1[10] by default
    // P2_8 = GPIO5[7] on function 4
    // P1_2 = GPIO0[9] by default
    // P1_1 = GPIO0[8] by default
    SFSP[2][8] = 4;
    GPIO_BYTE[1][10] = 0;
    GPIO_BYTE[5][7] = 1;
    GPIO_BYTE[0][9] = 0;
    GPIO_BYTE[0][8] = 1;
    GPIO_DIR[0] |= 0x300;
    GPIO_DIR[1] |= 1<<10;
    GPIO_DIR[5] |= 1<<7;
#endif

#if 0
    // Set m4 clock back to default...
    //* (v32*) 0x4005006c = 0x01000800;

    ser_w_hex (0, 0, "going...\r\n");

    typedef unsigned long long F22 (unsigned, unsigned);
    F22 * f = (F22*) 0x10401a87;

    unsigned X = f(0x40045000, 0);
    ser_w_hex ((unsigned) f, 8, " gives ");
    ser_w_hex (X, 8, "\r\n");
#endif

#if 0
    for (unsigned a = 0x10400000; a != 0x10410000;) {
        ser_w_hex(a, 8, " ");
        for (int i = 0; i < 16; ++i)
            ser_w_hex(* (unsigned char *) a++, 2,
                      i == 15 ? "\r\n" : " ");
    }
#endif

#if 0
    for (unsigned a = 0x40045000; a < 0x40045080;) {
        ser_w_hex(a, 8, " ");
        for (int i = 0; i < 8; ++i) {
            ser_w_hex(* (unsigned *) a, 8,
                      i == 7 ? "\r\n" : " ");
            a += 4;
        }
    }
#endif

#if 0
    // Call the boot-loader.
    ser_w_hex ((int) __builtin_return_address(0), 8, " return\r\n");
    ser_w_hex ((int) __builtin_frame_address(0), 8, " frame\r\n");

    // Set m4 clock back to default...
    * (v32*) 0x4005006c = 0x01000000;

//#if 0
    for (const char * p = "Going...\r\n"; *p; ++p)
        ser_w_byte(*p);
    //* (v32*) 0x40053104 = 0xffffffff; // Reset peripherals.
    // M4 reset, reset 13.  Master reset 2.  SCU 9.  GPIO 28.
    * (v32*) 0x40053100 = (1<<9) + (1<<28);

    typedef void F(void);
    F * fp = * (F**) 4;
    fp();
    /* * (v32*) 0x40043004 |= 32; */
    /* * (v32*) 0x400061a4 = 0x20; */
    /* ser_w_hex (* (v32*) 0x400061a4, 8, "\r\n"); */

    for (const char * p = "Did nothing...\r\n"; *p; ++p)
        ser_w_byte(*p);
#endif

#if 0
    // Power up the USB PHY.
    * (v32*) 0x40043004 &= ~32;

    // OTG register.
    ser_w_hex(* (v32*) 0x400061a4, 8, " OTG reg\r\n");
    ser_w_hex(* (v32*) 0x400061a8, 8, " USB Mode\r\n");
    // Set as device.
    * (v32*) 0x400061a8 = 2;
    for (volatile int i = 0; i < 1048576; ++i);
    ser_w_hex(* (v32*) 0x400061a4, 8, " OTG reg\r\n");
    ser_w_hex(* (v32*) 0x400061a8, 8, " USB Mode\r\n");
#endif

#if 1

    // Generate 480MHz off IRC...
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    * (v32*) 0x40050020 = 0x01000818;   // Control.
    * (v32*) 0x40050024 = 0x06167ffa;   // mdiv
    * (v32*) 0x40050028 = 0x00302062;   // np_div.

    for (volatile int i = 0; i != 100000; ++i);

    ser_w_hex (* (v32*) 0x4005001c, 8, " PLL0USB status.\r\n");

//    * (v32*) 0x40051428 = 0x1; // usb0 clk cntl.

    static const unsigned fakeotp[] = {
        0xa001c830, 0x008c4768, 0, 0x0ee40000, 0, 0, 0, 0,
        0, 0, 0, 0, 6 << 25, 0, 0, 0,
        0, 0, 0, 0, 0xfbf59a43, 0xbe72a0e3, 0xc14a1eec, 0x17a17db8,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    void * high = (void*) 0x10007000;
    mem_copy (high, (const unsigned char *) fakeotp, sizeof (fakeotp));

    typedef unsigned F (void*);

    ser_w_hex (((F*) 0x104020d1) (high), 8, " fake boot mode\r\n");

    ser_w_hex (0, 0, "Going....\r\n");
    ((F*) 0x1040158d) (high);
    ser_w_hex (0, 0, "Back....\r\n");
#endif
}
