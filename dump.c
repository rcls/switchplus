
#include "registers.h"

static void ser_w_byte (unsigned byte)
{
    while (!(*USART3_LSR & 32));         // Wait for THR to be empty.
    *USART3_THR = byte;
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


void _start (void) __section (".start");
void _start (void)
{
    GPIO_BYTE[4][1] = 1;
    GPIO_BYTE[4][2] = 0;
    GPIO_DIR[4] |= 6;
    GPIO_BYTE[4][1] = 0;
    GPIO_BYTE[4][2] = 1;

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

#if 1
    for (unsigned a = 0x1a000000; a != 0x1a000400;) {
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
}
