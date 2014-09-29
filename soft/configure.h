#ifndef CONFIGURE_H_
#define CONFIGURE_H_

// Helpers for configuring peripherals.  Table driven writes into peripheral
// addresses.

enum {
    op_zero,
    op_one,
    op_write32,
    op_spin,
    op_wait_zero,
    op_write
};

#define PIN(bank,num,function) WORD_WRITE(SFSP[bank][num], function)

#define PIN_OUT(a,b,f)      PIN(a,b,f)
#define PIN_OUT_FAST(a,b,f) PIN(a, b, 0x20 | (f))
#define PIN_IN(a,b,f)       PIN(a, b, 0x40 | (f))
#define PIN_IN_FAST(a,b,f)  PIN(a, b, 0xc0 | (f))

#define PIN_IO(a,b,f)       PIN_IN(a,b,f)
#define PIN_IO_FAST(a,b,f)  PIN(a, b, 0xe0 | (f))

#define opcode28(n,a) (((n) << 28) | (0xfffffff & (unsigned) (a)))
#define WORD_WRITE(a,v) opcode28(op_write, &(a)) + ((v)<<20) \
    + 0 * (1/((v) < 8 * 256))

#define WORD_WRITE32n(a,n,...) \
    opcode28(op_write32, &(a)) + ((n-1) << 20), ## __VA_ARGS__
#define WORD_WRITE32(a,v) opcode28(op_write32,&(a)), v

// Note that the 32* will overflow on the multiplication, stripping off leading
// bits.
#define opcode28bit(op,a,n) \
    opcode28(op, 0x42000000 + 32 * (unsigned) &(a) + 4 * (n))
#define BIT_SET(a,n)   opcode28bit(op_one, a, n)
#define BIT_RESET(a,n) opcode28bit(op_zero, a, n)

#define BIT_WAIT_ZERO(a,n) opcode28bit(op_wait_zero, a, n)

#define SPIN_FOR(n) opcode28(op_spin, n)

void configure(const unsigned * pins, int count);

void enter_dfu(void);
void check_for_early_dfu(void);

#endif
