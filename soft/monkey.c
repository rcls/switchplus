
#include "callback.h"
#include "configure.h"
#include "monkey.h"
#include "registers.h"
#include "usb.h"

#include <stdarg.h>
#include <stddef.h>

// The chip-select to the console, we configure as a GPIO.
#define CONSOLE_CS (&GPIO_WORD[7][19])


// Buffer for log text.
static unsigned char monkey_buffer[4096] __aligned (4096)
    __section ("ahb0.monkey_buffer");
#define monkey_buffer_end (monkey_buffer + sizeof monkey_buffer)

// Buffer for incoming characters.
static unsigned char monkey_recv[1024] __aligned (512)
    __section ("ahb0.monkey_recv");


// Ring-buffer descriptor.  We keep at least one byte free, so that
// insert==limit implies the buffer is empty.

// Position for inserting characters.
static unsigned char * insert_pos = monkey_buffer;
// Limit for inserting characters.
static unsigned char * limit_pos = monkey_buffer;

// First in-flight and next-to-queue over USB.
static unsigned char * usb_flight_pos = monkey_buffer;
static unsigned char * usb_send_pos = monkey_buffer;

// First in-flight and next-to-queue over SSP.
static unsigned char * ssp_flight_pos = monkey_buffer;
static unsigned char * ssp_send_pos = monkey_buffer;

static int monkey_in_next = -1;         // For ungetc.
static struct {
    unsigned char * next;
    unsigned char * end;
} monkey_recv_pos[2];


static void monkey_in_complete (dTD_t * dtd, unsigned status, unsigned remain);
static void monkey_out_complete (dTD_t * dtd, unsigned status, unsigned remain);

static void monkey_kick_usb(void);
static void monkey_kick_ssp(void);

bool debug_flag;
bool verbose_flag;
static bool log_ssp;


void init_monkey_usb (void)
{
    qh_init (EP_03, 0x20000000);        // No 0-size-frame on the monkey.
    qh_init (EP_83, 0x20000000);
    ENDPT->ctrl[3] = 0x00c800c8;

    usb_send_pos = usb_flight_pos;

    monkey_kick_usb();

    schedule_buffer (EP_03, monkey_recv, 512, monkey_out_complete);
    schedule_buffer (EP_03, monkey_recv + 512, 512, monkey_out_complete);

    monkey_recv_pos[0].next = 0;
    monkey_recv_pos[0].end  = 0;
    monkey_recv_pos[1].next = 0;
    monkey_recv_pos[1].end  = 0;
}


const unsigned init_monkey_regs[] __init_script("3") = {
    WORD_WRITE32(*BASE_SSP1_CLK, 0x0c000800), // Base clock is be 160MHz.

    // Reset ssp1, keep m0 in reset.
    WORD_WRITE32(RESET_CTRL[1], (1 << 19) | (1 << 24)),
    BIT_WAIT_SET(RESET_ACTIVE_STATUS[1], 19),

    WORD_WRITE(SSP1->cpsr, 2),          // Clock pre-scale: 160MHz / 2 = 80MHz.
    // 8-bit xfer, clock low between frames, capture on first (rising) edge of
    // frame (we'll output on falling edge).  No divide.
    WORD_WRITE(SSP1->cr0, 0x0007),
    WORD_WRITE(SSP1->cr1, 2),           // Enable master.

    WORD_WRITE(SSP1->dmacr, 2),         // TX DMA enabled.

    // Setup pins; make CS a GPIO output, pulse it high for a bit.
    BIT_SET(GPIO_DIR[7], 19),
    // Leave CS high, monkey_ssp_on() actually turns it on.
    WORD_WRITE(*CONSOLE_CS, 1),

    PIN_OUT_FAST(15,4,0),               // SCK is D10, PF_4 func 0.
    PIN_OUT_FAST(15,5,4),               // SSEL is E9, PF_5, GPIO7[19] func 4.
    PIN_IO_FAST (15,6,2),               // MISO is E7, PF_6 func 2.
    PIN_OUT_FAST(15,7,2),               // MOSI is B7, PF_7 func 2.

    WORD_WRITE(GPDMA->config, 1),   // Enable.
};


void monkey_ssp_off(void)
{
    __interrupt_disable();
    while (ssp_send_pos != ssp_flight_pos)
        __interrupt_wait_go();
    log_ssp = false;
    __interrupt_enable();

    // Wait for idle & clear out the fifo..
    while (SSP1->sr & 20)
        SSP1->dr;

    *CONSOLE_CS = 1;
}


void monkey_ssp_on(void)
{
    *CONSOLE_CS = 0;

    __interrupt_disable();
    log_ssp = true;
    monkey_kick_ssp();
    __interrupt_enable();
}


// At low priority, we treat buffer full by waiting for interrupts.  At high
// priority we spin or drop.
static inline bool at_low_priority(void)
{
    unsigned r;
    asm volatile ("\tmrs %0,ipsr\n" : "=r"(r));
    return r == 0 || r > 32;
}


static inline void enter_monkey(void)
{
    __interrupt_disable();
}


static void leave_monkey(void)
{
    monkey_kick_ssp();
    monkey_kick_usb();
    __interrupt_enable();
}

static inline unsigned min(unsigned x, unsigned y)
{
    return x < y ? x : y;
}


static inline unsigned headroom(unsigned char * p)
{
    return (p - insert_pos - 1) & 4095;
}


static unsigned char * advance(unsigned char * p, int amount)
{
    return monkey_buffer + ((amount + (int) p) & 4095);
}


static void free_monkey_space_usb(void)
{
    // Running at high priority : discard.
    endpt_complete_one(EP_83);
    if (headroom(usb_flight_pos) != 0)
        return;

    // Last resort - just bump the pointers.
    unsigned char * old = usb_flight_pos;
    usb_flight_pos = advance(old, 512);
    if (((usb_send_pos - old) & 4095) < 512)
        usb_send_pos = usb_flight_pos;
}


static void free_monkey_space_ssp(void)
{
    // We just did a kick, so if the buffer pointers are equal, that is
    // because ssp is off.  In that case, just advance pointers.
    if (ssp_flight_pos == ssp_send_pos) {
        ssp_send_pos = advance(ssp_send_pos, 512);
        ssp_flight_pos = ssp_send_pos;
        return;
   }

    // We could just make the GPDMA interrupt higher priority?
    do
        gpdma_interrupt();
    while (headroom(ssp_flight_pos) == 0);
}


static void free_monkey_space(void)
{
    leave_monkey();                     // Includes kick.
    enter_monkey();

    // Recalculate buffer positions...  Note that enabling interrupts can
    // invalidate everything, meaning we need to redo the entire calculation.
retry: ;
    unsigned allowed_usb = headroom(usb_flight_pos);
    unsigned allowed_ssp = headroom(ssp_flight_pos);

    if (allowed_usb == 0 || allowed_ssp == 0) {
        if (at_low_priority()) {        // Low priority - wait and retry.
            __interrupt_wait_go();
            goto retry;
        }
        if (allowed_ssp == 0)
            free_monkey_space_ssp();
        if (allowed_usb == 0)
            free_monkey_space_usb();
        goto retry;
    }

    insert_pos = advance(insert_pos, 0); // Wrap.

    unsigned allowed = 512;
    allowed = min(allowed, allowed_usb);
    allowed = min(allowed, allowed_ssp);

    allowed = min(allowed, monkey_buffer_end - insert_pos);

    limit_pos = insert_pos + allowed;
}


static void write_byte (int byte)
{
    if (__builtin_expect(insert_pos == limit_pos, 0))
        free_monkey_space();

    *insert_pos++ = byte;
}


void putchar (int byte)
{
    enter_monkey();
    write_byte (byte);
    leave_monkey();
}


void puts (const char * s)
{
    enter_monkey();
    for (; *s; s++)
        write_byte (*s);
    leave_monkey();
}


void monkey_kick_ssp(void)
{
    if (!log_ssp || ssp_flight_pos != ssp_send_pos)
        return;                         // Not enabled or not idle.

    unsigned amount = 512;
    amount = min(amount, insert_pos - ssp_send_pos);
    amount = min(amount, monkey_buffer_end - ssp_send_pos);

    if (amount == 0)
        return;

    // With mux option 0, SSP1 TX is peripheral 12.
    volatile gpdma_channel_t * channel = &GPDMA->channel[0];
    channel->srcaddr = ssp_send_pos;
    channel->destaddr = &SSP1->dr;
    channel->lli = NULL;
    // Bit 12..14 : Src burst size 1 = 4 bytes.
    // Bit 15..17 : Dst burst size 1 = 4 bytes.
    // Bit 18..20 : Swidth 0/1/2 = 8/16/32 bit
    // Bit 21..22 : Dwidth 0/1/2 = 8/16/32 bit
    // Bit 24 : src on master1.
    // Bit 25 : dest on master1; only master1 can access peripherals.
    // Bit 26 : source address increment.
    // Bit 28 : priviledged mode.
    // Bit 31 : terminal count interrupt.
    channel->control = (1<<31) + (1<<28) + (1<<26) + (1<<25)
        + (1<<15) + (1<<12) + amount;
    // Bit 0 : Enable.
    // Bits 10..6 : Dest periph = 12.
    // Bits 13..11 : Flow control = 1, mem to perip, DMA control.
    // Bit 14 : Error interrupt enable.
    // Bit 15 : Terminal interrupt enable.
    channel->config = (1 << 15) + (1 << 14) + (1 << 11) + (12 << 6) + 1;

    ssp_send_pos = advance(ssp_send_pos, amount);
}


void gpdma_interrupt(void)
{
    unsigned tcstat = GPDMA->inttcstat;
    GPDMA->inttcclear = tcstat;
    unsigned tcerr = GPDMA->interrstat;
    GPDMA->interrclr = tcerr;

    if ((tcstat | tcerr) & 1) {         // Channel just finished.
        ssp_flight_pos = ssp_send_pos;
        monkey_kick_ssp();
    }
}


static void monkey_kick_usb(void)
{
    if (!(ENDPT->ctrl[3] & 0x800000))   // Short circuit if not active.
        return;

    while (1) {
        unsigned avail = (insert_pos - usb_send_pos) & 4095;
        // Send at most 512 bytes.  Send only if either we are idle or have a
        // full 512 bytes.
        if (avail >= 512)
            avail = 512;
        else if (!avail || usb_flight_pos != usb_send_pos)
            return;

        dTD_t * dtd = get_dtd();
        dtd->buffer_page[0] = usb_send_pos;
        dtd->buffer_page[1] = monkey_buffer; // Cyclic.

        usb_send_pos = advance(usb_send_pos, avail);
        dtd->buffer_page[4] = usb_send_pos;

        dtd->length_and_status = avail * 65536 + 0x8080;
        dtd->completion = monkey_in_complete;
        schedule_dtd (EP_83, dtd);
    }
}


bool monkey_is_empty (void)
{
    __memory_barrier();                 // Called without interrupts disabled...
    return usb_flight_pos == usb_send_pos;
}


static void monkey_in_complete (dTD_t * dtd, unsigned status, unsigned remain)
{
    // On any completion except for shutdown, assume that we want to drop the
    // data.  Also, if the buffer is full, then drop the data.
    if (status != 0x80 || headroom(usb_flight_pos) == 0)
        usb_flight_pos = dtd->buffer_page[4];

    monkey_kick_usb();
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
    unsigned char c[12];
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

    for (; width > p - c; --width)
        write_byte (fill);

    do
        write_byte (*--p);
    while (p != c);
}


void printf (const char * restrict f, ...)
{
    enter_monkey();
    va_list args;
    va_start (args, f);

    for (const char * s = f; *s; ++s) {
        if (*s != '%') {
            write_byte(*s & 0xff);
            continue;
        }

        ++s;
        unsigned char fill = ' ';
        if (*s == '0')
            fill = '0';

        unsigned width = 0;
        for (; *s >= '0' && *s <= '9'; ++s)
            width = width * 10 + *s - '0';
        unsigned base;
        bool sgn = false;
        unsigned lng = 0;
        for (; *s == 'l'; ++s)
            ++lng;
        unsigned lower = *s & 32;
        switch (*s | 32) {
        case 'x':
            lower = 0x20;
            base = 16;
            break;
        case 'i':
        case 'd':
            sgn = true;
        case 'u':
            base = 10;
            break;
        case 'p':
            base = 16;
            width = 8;
            fill = '0';
            break;
        case 's':
            format_string (va_arg (args, const char *), width, fill);
            continue;
        default:
            continue;
        }

        unsigned long value = va_arg(args, unsigned long);
        format_number (value, base, lower, sgn, width, fill);
    }

    va_end (args);

    leave_monkey();
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
    schedule_buffer (EP_03, monkey_recv_pos->end - offset, 512,
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


static void monkey_out_complete (dTD_t * dtd, unsigned status, unsigned remain)
{
    unsigned char * buffer = dtd->buffer_page[4];
    unsigned length = 512 - remain;

    if (length == 0 || status != 0) {
        // Reschedule immediately.
        if (ENDPT->ctrl[3] & 0x80)
            schedule_buffer (EP_03, buffer, 512, monkey_out_complete);
        return;
    }

    int index = 0;
    if (monkey_recv_pos->next != NULL)
        index = 1;

    monkey_recv_pos[index].next = buffer;
    monkey_recv_pos[index].end = buffer + length;
}


int hex_nibble(int c)
{
    if (c >= '0' && c <= '9') {
        if (verbose_flag)
            putchar(c);
        return c - '0';
    }
    c &= ~32;
    if (c >= 'A' && c <= 'F') {
        if (verbose_flag)
            putchar(c);
        return c - 'A' + 10;
    }
    printf(CLR "Illegal hex character; aborting...");
    if (c != '\n')
        while (getchar() != '\n');
    putchar('\n');
    restart_program(NULL);
}
