// We have leds on H5 (P8_1, GPIO4[1]) and K4 (P8_2, GPIO4[2]).

#include "registers.h"

void doit (void)
{
    GPIO_BYTE[4][1] = 1;
    GPIO_BYTE[4][2] = 0;
    GPIO_DIR[4] |= 6;

    int dir = 0;
    while (1) {
        for (volatile int i = 0; i != 1048576; ++i);
        GPIO_BYTE[4][1] = dir;
        dir ^= 1;
        GPIO_BYTE[4][2] = dir;
    }
}

unsigned start[256] __attribute__ ((section (".start")));
unsigned start[256] = {
    0x10089ff0,
    (unsigned) doit
};