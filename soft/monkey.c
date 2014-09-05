
#include "callback.h"
#include "monkey.h"
#include "registers.h"
#include "usb.h"

#include <stdarg.h>
#include <stddef.h>

#define CONSOLE_CS (&GPIO_BYTE[7][19])

// Ring-buffer descriptor.  insert==limit implies the buffer is full; we set
// insert=beginning, limit=NULL when empty.  The area from insert to limit is
// either in-flight or waiting to be sent.
static struct {
    unsigned char * insert;             // Where to insert new characters.
    unsigned char * limit;              // End of insertion region.
    bool outstanding;                   // Do we have a USB xmit outstanding?
} monkey_pos;


static int monkey_in_next;
static struct {
    unsigned char * next;
    unsigned char * end;
} monkey_recv_pos[2];


static unsigned char monkey_buffer[4096] __aligned (4096)
    __section ("ahb0.monkey_buffer");
#define monkey_buffer_end (monkey_buffer + sizeof monkey_buffer)

static unsigned char monkey_recv[1024] __aligned (512)
    __section ("ahb0.monkey_recv");

static void monkey_in_complete (dTD_t * dtd);
static void monkey_out_complete (dTD_t * dtd);

bool debug_flag;
bool verbose_flag;
bool log_ssp;


void init_monkey_usb (void)
{
    if (log_monkey)
        monkey_kick();

    schedule_buffer (3, monkey_recv, 512, monkey_out_complete);
    schedule_buffer (3, monkey_recv + 512, 512, monkey_out_complete);
    monkey_in_next = -1;
}


void init_monkey_ssp (void)
{
    log_ssp = false;
    __memory_barrier();

    *BASE_SSP1_CLK = 0x0c000800;        // Base clock is 160MHz.

    RESET_CTRL[1] = (1 << 19) | (1 << 24); // Reset ssp1, keep m0 in reset.
    while (!(RESET_ACTIVE_STATUS[1] & (1 << 19)));

    SSP1->cpsr = 2;                    // Clock pre-scale: 160MHz / 2 = 80MHz.
    // 8-bit xfer, clock low between frames, capture on first (rising) edge of
    // frame (we'll output on falling edge).  No divide.
    SSP1->cr0 = 0x0007;
    SSP1->cr1 = 2;                      // Enable master.
    // Setup pins; make CS a GPIO output, pulse it high for a bit.
    GPIO_DIR[7] |= 1 << 19;
    *CONSOLE_CS = 1;

    SFSP[15][4] = 0x20;                 // SCK is D10, PF_4 func 0.
    SFSP[15][5] = 0x24;                 // SSEL is E9, PF_5, GPIO7[19] func 4.
    SFSP[15][6] = 0xe2;                 // MISO is E7, PF_6 func 2.
    SFSP[15][7] = 0x22;                 // MOSI is B7, PF_7 func 2.

    // Leave CS low.
    *CONSOLE_CS = 0;

    __memory_barrier();
    log_ssp = true;
}


static inline unsigned current_irs (void)
{
    unsigned r;
    asm ("\tmrs %0,psr\n" : "=r"(r));
    return r & 511;
}


static inline unsigned enter_monkey (void)
{
    unsigned r = current_irs();
    if (!r)
        __interrupt_disable();
    return r;
}


static inline void leave_monkey (unsigned r)
{
    if (log_monkey)
        monkey_kick();
    if (!r)
        __interrupt_enable();
}


static void write_byte (int byte)
{
    if (log_ssp) {
        // FIXME - also poll the gpio used for overflow.
        while (!(SSP1->sr & 2));
        SSP1->dr = byte;
    }

    if (!log_monkey)
        return;

    if (monkey_pos.limit == NULL) {
        monkey_pos.limit = monkey_buffer;
        monkey_pos.insert = monkey_buffer;
        *monkey_pos.insert++ = byte;
        return;
    }

    while (monkey_pos.insert == monkey_pos.limit) {
        // If we're in an interrupt handler, or there is nothing outstanding,
        // drop the data.
        monkey_kick();
        if (!monkey_pos.outstanding || current_irs() != 0)
            return;                     // Full
        __interrupt_wait();
        __interrupt_enable();
        __interrupt_disable();
    }

    *monkey_pos.insert++ = byte;
    if (monkey_pos.insert == monkey_buffer_end)
        monkey_pos.insert = monkey_buffer;
}


void putchar (int byte)
{
    unsigned l = enter_monkey();
    write_byte (byte);
    leave_monkey (l);
}


void puts (const char * s)
{
    unsigned l = enter_monkey();
    for (; *s; s++)
        write_byte (*s);
    leave_monkey (l);
}


void monkey_kick (void)
{
    // If the dTD is in-flight, or there is no data, or the monkey end-point is
    // not in use, then nothing to do.
    if (!log_monkey || monkey_pos.outstanding
        || monkey_pos.limit == NULL || !(*ENDPTCTRL3 & 0x800000))
        return;

    // FIXME - we should do something to get stuff out on out-of-dtds.
    dTD_t * dtd = get_dtd();
    dtd->buffer_page[0] = (unsigned) monkey_pos.limit;
    dtd->buffer_page[1] = (unsigned) monkey_buffer; // Cyclic.
    int length = monkey_pos.insert - monkey_pos.limit;
    if (length <= 0)
        length += 4096;
    dtd->length_and_status = length * 65536 + 0x8080;
    dtd->completion = monkey_in_complete;
    schedule_dtd (0x83, dtd);
    monkey_pos.outstanding = true;
}


bool monkey_is_empty (void)
{
    unsigned char * volatile * p = &monkey_pos.limit;
    return *p == NULL;
}


static void monkey_in_complete (dTD_t * dtd)
{
    // FIXME - distinguish between errors and sending a full page?
    monkey_pos.outstanding = false;
    monkey_pos.limit = (unsigned char *) dtd->buffer_page[0];
    if (monkey_pos.limit == monkey_pos.insert)
        // We're idle.  Reset the pointers.
        monkey_pos.limit = NULL;
    else
        monkey_kick();
}


static void format_string (const char * s, unsigned width, unsigned char fill)
{
    for (const char * e = s; *e; ++e, --width)
        if (width == 0)
            break;
    for (; width != 0; --width)
        write_byte (fill);
    for (; *s; ++s)
        write_byte (*s);
}


static void format_number (unsigned long value, unsigned base, unsigned lower,
                           bool sgn, unsigned width, unsigned char fill)
{
    unsigned char c[23];
    unsigned char * p = c;
    if (sgn && (long) value < 0)
        value = -value;
    else
        sgn = false;

    do {
        unsigned digit = value % base;
        if (digit >= 10)
            digit += 'A' - '0' - 10 + lower;
        *p++ = digit + '0';
        value /= base;
    }
    while (value);

    if (!sgn)
        ;
    else if (fill == ' ')
        *p++ = '-';
    else {
        write_byte ('-');
        if (width > 0)
            --width;
    }

    while (width > p - c) {
        write_byte (fill);
        --width;
    }

    while (p != c)
        write_byte (*--p);
}


void printf (const char * restrict f, ...)
{
    unsigned l = enter_monkey();
    va_list args;
    va_start (args, f);
    const unsigned char * s;

    for (s = (const unsigned char *) f; *s; ++s) {
        switch (*s) {
        case '%': {
            ++s;
            unsigned char fill = ' ';
            if (*s == '0')
                fill = '0';

            unsigned width = 0;
            for (; *s >= '0' && *s <= '9'; ++s)
                width = width * 10 + *s - '0';
            unsigned base = 0;
            unsigned lower = 0;
            bool sgn = false;
            unsigned lng = 0;
            for (; *s == 'l'; ++s)
                ++lng;
            switch (*s) {
            case 'x':
                lower = 0x20;
            case 'X':
                base = 16;
                break;
            case 'i':
            case 'd':
                sgn = true;
            case 'u':
                base = 10;
                break;
            case 'o':
                base = 8;
                break;
            case 'p': {
                void * value = va_arg (args, void *);
                if (width == 0)
                    width = 8;
                format_number ((unsigned) value, 16, 32, false, width, '0');
                break;
            }
            case 's':
                format_string (va_arg (args, const char *), width, fill);
                break;
            }
            if (base != 0) {
                unsigned long value;
                if (lng)
                    value = va_arg (args, unsigned long);
                else if (sgn)
                    value = va_arg (args, int);
                else
                    value = va_arg (args, unsigned);
                format_number (value, base, lower, sgn, width, fill);
            }
            break;
        }

        default:
            write_byte (*s);
        }
    }

    va_end (args);

    leave_monkey (l);
}


int getchar (void)
{
    // In case we've done an unget...
    int r = monkey_in_next;
    monkey_in_next = -1;
    if (r >= 0)
        return r;

    unsigned char * n = monkey_recv_pos->next;
    __memory_barrier();
    if (n == NULL) {
        __interrupt_disable();
        while (monkey_recv_pos->next == NULL)
            callback_wait();
        n = monkey_recv_pos->next;
        __interrupt_enable();
    }

    r = *n++;
    monkey_recv_pos->next = n;
    if (n != monkey_recv_pos->end)
        return r;

    __interrupt_disable();
    unsigned offset = (511 & ((long) monkey_recv_pos->end - 1)) + 1;
    schedule_buffer (3, monkey_recv_pos->end - offset, 512,
                     monkey_out_complete);
    monkey_recv_pos[0] = monkey_recv_pos[1];
    monkey_recv_pos[1].next = NULL;
    monkey_recv_pos[1].end = NULL;
    __interrupt_enable();
    return r;
}


int peekchar_nb (void)
{
    int r = monkey_in_next;
    if (r == -1 && monkey_recv_pos->next)
        r = *monkey_recv_pos->next;
    return r;
}


void ungetchar (int c)
{
    monkey_in_next = c & 255;
}


static void monkey_out_complete (dTD_t * dtd)
{
    unsigned length_and_status = dtd->length_and_status;
    unsigned char * buffer = (unsigned char *) dtd->buffer_page[4];
    unsigned length = 512 - (length_and_status >> 16);

    if (length == 0 || (length_and_status & 0x80)) {
        // Reschedule immediately.
        if (*ENDPTCTRL3 & 0x80)
            schedule_buffer (3, buffer, 512, monkey_out_complete);
        return;
    }

    int index = 0;
    if (monkey_recv_pos->next != NULL)
        index = 1;

    monkey_recv_pos[index].next = buffer;
    monkey_recv_pos[index].end = buffer + length;
}
