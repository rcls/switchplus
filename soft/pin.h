#ifndef PIN_H_
#define PIN_H_

// Helpers for configuring pins.  We pack the pin config into a 32-bit word.
// Top top 16 bits are the pin number, the bottom 16 bits are the config.

#define PIN(bank,num,function) (((bank) << 21) + ((num) << 16) + (function))

#define PIN_OUT(a,b,f)      PIN(a,b,f)
#define PIN_OUT_FAST(a,b,f) PIN(a, b, 0x20 | (f))
#define PIN_IN(a,b,f)       PIN(a, b, 0x40 | (f))
#define PIN_IN_FAST(a,b,f)  PIN(a, b, 0xc0 | (f))

#define PIN_IO(a,b,f)       PIN_IN(a,b,f)
#define PIN_IO_FAST(a,b,f)  PIN(a, b, 0xe0 | (f))

void config_pins(const unsigned * pins, int count);

void enter_dfu(void);
void check_for_early_dfu(void);

#endif
