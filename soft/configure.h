#ifndef CONFIGURE_H_
#define CONFIGURE_H_

// Helpers for configuring peripherals.  Table driven writes into peripheral
// addresses.

#define PIN(bank,num,function) WORD_WRITE(SFSP[bank][num], function)

#define PIN_OUT(a,b,f)      PIN(a,b,f)
#define PIN_OUT_FAST(a,b,f) PIN(a, b, 0x20 | (f))
#define PIN_IN(a,b,f)       PIN(a, b, 0x40 | (f))
#define PIN_IN_FAST(a,b,f)  PIN(a, b, 0xc0 | (f))

#define PIN_IO(a,b,f)       PIN_IN(a,b,f)
#define PIN_IO_FAST(a,b,f)  PIN(a, b, 0xe0 | (f))

#define opcode28(n,a) (((n) << 28) | (0xfffffff & (unsigned) (a)))
#define BYTE_ZERO(a) opcode28(0,&(a))
#define BYTE_ONE(a)  opcode28(1,&(a))
#define WORD_WRITE(a,v) opcode28(3,&(a)) + ((v)<<20) + 0 * (1/((v) < 768))
//#define WORD_WRITE32(a,v) opcode28(2,a), v

// Note that the 32* will overflow on the multiplication, stripping off leading
// bits.
#define BIT_SET(a,n)   opcode28(1, 0x42000000 + 32 * (unsigned) &(a) + 4 * (n))
#define BIT_RESET(a,n) opcode28(0, 0x42000000 + 32 * (unsigned) &(a) + 4 * (n))

void configure(const unsigned * pins, int count);

void enter_dfu(void);
void check_for_early_dfu(void);

#endif
