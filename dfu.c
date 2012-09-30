// Just bounce from the serial ISP to USB0 DFU.

#include "registers.h"

static void ser_w_byte (unsigned byte)
{
    while (!(*USART3_LSR & 32));         // Wait for THR to be empty.
    *USART3_THR = byte;
}

static void ser_w_string(const char * term)
{
    for (; *term; ++term)
        ser_w_byte(*term);
}

void _start (void) __attribute__ ((section (".start")));
void _start (void)
{
    GPIO_DIR[4] |= 6;
    GPIO_BYTE[4][1] = 0;
    GPIO_BYTE[4][2] = 1;

    // Generate 480MHz off IRC...
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    * (v32*) 0x40050020 = 0x01000818;   // Control.
    * (v32*) 0x40050024 = 0x06167ffa;   // mdiv
    * (v32*) 0x40050028 = 0x00302062;   // np_div.

    for (volatile int i = 0; i != 100000; ++i);

    unsigned * high = (unsigned*) 0x10007000;
    for (int i = 0; i != 32; ++i)
        high[i] = OTP[i];

    high[12] = 6 << 25;

    typedef unsigned F (void*);

    ser_w_string ("Going....\r\n");
    ((F*) 0x1040158d) (high);
    ser_w_string ("Back....\r\n");
}
