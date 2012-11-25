
#include "monkey.h"
#include "registers.h"
#include "usb.h"

#include <stdarg.h>
#include <stddef.h>

// Ring-buffer descriptor.  insert==limit implies the buffer is full; we set
// insert=beginning, limit=NULL when empty.  The area from insert to limit is
// either in-flight or waiting to be sent.
static struct {
    unsigned char * insert;             // Where to insert new characters.
    unsigned char * limit;              // End of insertion region.
    bool outstanding;                   // Do we have a USB xmit outstanding?
} monkey_pos;

static unsigned char monkey_buffer[4096] __aligned (4096)
    __section ("ahb0.monkey_buffer");
#define monkey_buffer_end (monkey_buffer + sizeof monkey_buffer)

static void monkey_in_complete (dTD_t * dtd);

bool log_serial;
bool debug_flag;

void init_monkey_serial (void)
{
    // Bring up USART3.
    SFSP[2][3] = 2;                     // P2_3, J12 is TXD on function 2.
    SFSP[2][4] = 0x42;                  // P2_4, K11 is RXD on function 2.

    // Set the USART3 clock to be the 12MHz IRC.
    *BASE_UART3_CLK = 0x01000800;

    *USART3_LCR = 0x83;                 // Enable divisor access.

    // From the user-guide: Based on these findings, the suggested USART setup
    // would be: DLM = 0, DLL = 4, DIVADDVAL = 5, and MULVAL = 8.
    *USART3_DLM = 0;
    *USART3_DLL = 4;
    *USART3_FDR = 0x85;

    *USART3_LCR = 0x3;                  // Disable divisor access, 8N1.
    *USART3_FCR = 1;                    // Enable fifos.
    *USART3_IER = 1;                    // Enable receive interrupts.
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
    if (log_serial) {
        while (!(*USART3_LSR & 32));    // Wait for THR to be empty.
        *USART3_THR = byte;
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
