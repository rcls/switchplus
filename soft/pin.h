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

#define opcode28(n,a) (((n) << 28) | (0xfffffff & (unsigned) (a)))
#define BYTE_ZERO(a) opcode28(2,a)
#define BYTE_ONE(a)  opcode28(3,a)
#define WORD_WRS(a,v) opcode28(4,a) + (v<<20)
#define WORD_ZERO(a) WORD_WRS(a,0)
#define WORD_ONE(a)  WORD_WRS(a,1)
#define WORD_WRITE(a,v) opcode28(1,a), v

#define BIT_SET(a,n)   BYTE_ONE (0x42000000 + 32 * (unsigned) (a) + 4 * (n))
#define BIT_RESET(a,n) BYTE_ZERO(0x42000000 + 32 * (unsigned) (a) + 4 * (n))

#define STORE_ONE(a) sizeof(*a) == 1 ? BYTE_ONE(a) : WORD_ONE(a)
#define STORE_ZERO(a) sizeof(*a) == 1 ? BYTE_ZERO(a) : WORD_ZERO(a)

void config_pins(const unsigned * pins, int count);

void enter_dfu(void);
void check_for_early_dfu(void);

#endif
