#include "pin.h"

void config_pins(const unsigned * pins, int count)
{
    volatile unsigned * sfsp = (volatile unsigned *) 0x40086000;
    for (int i = 0; i < count; ++i) {
        unsigned pin = pins[i] >> 16;
        unsigned config = pins[i] & 0xffff;
        sfsp[pin] = config;
    }
}
