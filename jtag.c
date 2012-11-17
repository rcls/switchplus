
#include "callback.h"
#include "monkey.h"
#include "registers.h"
#include "usb.h"

#define TCK GPIO_BYTE[6][13]
#define TDO GPIO_BYTE[5][3]
#define TDI GPIO_BYTE[6][12]
#define TMS GPIO_BYTE[5][4]

#define JTAG_EP_IN 0x84
#define JTAG_EP_OUT 0x04

static unsigned char usb_buffer[1024] __aligned (512) __section ("ahb0.jtag");

static callback_record_t run_commands_cb[2];
static void jtag_rx_callback (callback_record_t *);

static void wait(void)
{
    for (int i = 0; i != 25; ++i)
        asm volatile ("");
}


static int jtag_clk(int tms, int tdi)
{
    TDI = !!tdi;
    TMS = !!tms;
    wait();
    TCK = 0;
    wait();
    int r = TDO;
    TCK = 1;
    return r;
}


static void jtag_clk_and_store(int tms, int tdi,
                               unsigned char * dest, unsigned i)
{
    if (jtag_clk(tms, tdi))
        dest[i / 8] |= 1 << (i % 8);
    else
        dest[i / 8] &= ~(1 << (i % 8));
}


void jtag_reset(void)
{
    log_serial = false;                 // We share pins with the serial log...

    // Set the jtag pins to be GPIO.
    // TCK is GPIO6[13] PC_14 func 4 ball N1
    // TDO is GPIO5[3] P2_3 func 4 ball J12
    // TDI is GPIO6[12] PC_13 func 4 ball M1
    // TMS is GPIO5[4] P2_4 func 4 ball K11

    GPIO_DIR[6] |= 0x3000;
    GPIO_DIR[5] = (GPIO_DIR[5] & ~8) | 0x10;

    TCK = 0;
    TDI = 1;
    TMS = 1;

    SFSP[12][14] = 4;
    SFSP[2][3]   = 0x44;
    SFSP[12][13] = 4;
    SFSP[2][4]   = 4;

    // Reset jtag.
    for (int i = 0; i != 10; ++i)
        jtag_clk(1,1);

    jtag_clk(0,1);                      // Goto run test idle.
    jtag_clk(1,1);                      // Goto select DR.
    jtag_clk(0,1);                      // Goto capture DR.
    jtag_clk(0,1);                      // Goto shift DR.
    unsigned code = 0;
    for (int i = 0; i != 32; ++i)
        code |= jtag_clk(0,1) << i;
    printf ("idcode %08x\n", code);

    // Reset again.
    for (int i = 0; i != 10; ++i)
        jtag_clk(1,1);
}

// Opcodes: ooooxxxx
// 000000 : single tms+tdi, no capture.
// 000100 : single tms+tdi, capture.
// 000x0100 : jtag reset off.
// 000x0101 : jtag reset on.
// 000x0110 : jtag reset sample.
// 000x0111 : TDO sample.
// 0010 : double tms+tdi, no capture.
// 0011 : double tms+tdi, capture.
// 0100 : short tms+tdi, no capture.
// 0101 : short tms+tdi, capture.
// 0110 : long tdi, no capture.
// 0111 : long tdi, capture.
// 1111 : Ping (0 to 15 bytes payload).

// Return opcodes:
// 0001 : single.
// 0011 : double
// 0101 : short.
// 0111 : long.


static const unsigned char * jtag_single(const unsigned char * p,
                                         const unsigned char * end,
                                         unsigned char ** output)
{
    unsigned char * result = *output;
    unsigned bit = 0;
    switch (*p & 15) {
    case 0: case 1: case 2: case 3:     // Clock tms & tdi.
        bit = jtag_clk (*p & 2, *p & 1);
        break;
    case 4:                             // TRST off.
        break;
    case 5:                             // TRST on.
        jtag_reset();
        bit = 1;
        break;
    case 6:                             // TRST sample.
        bit = 0;
        break;
    case 7:                             // Sample TDO.
        // Just in case the JTAG waits for TCK falling edge, take TCK low
        // and wait a bit.
        TCK = 0;
        wait();
        bit = TDO;
        break;
    }

    *result++ = 0x10 + bit;
    if (*p & 16)
        *output = result;
    return p + 1;
}


static const unsigned char * jtag_double(const unsigned char * p,
                                         const unsigned char * end,
                                         unsigned char ** output)
{
    unsigned char * result = *output;
    unsigned bit0 = jtag_clk (*p & 2, *p & 1);
    unsigned bit1 = jtag_clk (*p & 8, *p & 4);
    *result++ = 0x30 + bit1 * 2 + bit0;
    if (*p & 16)
        *output = result;
    return p + 1;
}


static const unsigned char * jtag_short(const unsigned char * p,
                                        const unsigned char * end,
                                        unsigned char ** output)
{
    unsigned char * result = *output;
    unsigned opcode = *p++;
    *result++ = opcode;

    unsigned length = (opcode & 15) + 2;
    unsigned bytes = (length + 3) / 4;
    if (end - p < bytes)
        return end;

    for (unsigned i = 0; i != length; ++i) {
        unsigned mask = 1 << 2 * (i % 4);
        jtag_clk_and_store (p[i / 4] & (mask * 2), p[i / 4] & mask,
                            result, 2 * i);
    }

    if (*p & 16)
        *output = result + bytes;
    return p + bytes;
}


static const unsigned char * jtag_long(const unsigned char * p,
                                       const unsigned char * end,
                                       unsigned char ** output)
{
    if (end - p < 2)
        return end;

    unsigned char * result = *output;
    unsigned opcode = *p++;
    *result++ = opcode;
    *result++ = *p;
    unsigned length = (opcode & 15) + *p++ * 16;
    unsigned bytes = (length + 7 / 8);
    if (end - p < bytes)
        return end;


    for (unsigned i = 0; i != length; ++i)
        jtag_clk_and_store(0, p[i / 8] & (1 << i % 8), result, i);

    if (*p & 16)
        *output = result + bytes;
    return p + bytes;
}


static const unsigned char * jtag_ping (const unsigned char * p,
                                        const unsigned char * end,
                                        unsigned char ** output)
{
    unsigned length = (*p & 15) + 1;
    if (end - p < length)
        return end;
    unsigned char * result = *output;
    for (unsigned i = 0; i != length; ++i)
        *result++ = *p++;
    *output = result;
    return p;
}


static const unsigned char * jtag_command (const unsigned char * p,
                                           const unsigned char * end,
                                           unsigned char ** output)
{
    if (p == end)
        return p;

    switch (*p >> 4) {
    case 0: case 1:
        return jtag_single(p, end, output);
    case 2: case 3:
        return jtag_double(p, end, output);
    case 4: case 5:
        return jtag_short(p, end, output);
    case 6: case 7:
        return jtag_long(p, end, output);
    case 15:
        return jtag_ping(p, end, output);
    default:
        return end;
    }
}


static void jtag_rx_complete(dTD_t * dtd)
{
    // Run jtag_commands outside of interrupt...
    callback_record_t * rec;
    if ((void*) dtd->buffer_page[4] == usb_buffer)
        rec = &run_commands_cb[0];
    else
        rec = &run_commands_cb[1];
    rec->udata = 512 - (dtd->length_and_status >> 16);
    callback_schedule (jtag_rx_callback, rec);
}


static void jtag_tx_complete(dTD_t * dtd)
{
    schedule_buffer(JTAG_EP_OUT,
                    (void *) dtd->buffer_page[4], 512,
                    jtag_rx_complete);
}


static void jtag_commands (unsigned char * p, unsigned length)
{
    unsigned char * end = p + length;
    unsigned char * result = p;
    for (const unsigned char * q = p; q != end;)
        q = jtag_command (q, end, &result);

    asm volatile ("\tcpsid i\n" ::: "memory");
    if (result != p)
        schedule_buffer (JTAG_EP_IN, p, result - p, jtag_tx_complete);
    else
        schedule_buffer (JTAG_EP_OUT, p, 512, jtag_rx_complete);
    asm volatile ("\tcpsie i\n" ::: "memory");
}

static void jtag_rx_callback (callback_record_t * record)
{
    unsigned char * buffer;
    if (record == &run_commands_cb[0])
        buffer = usb_buffer;
    else
        buffer = usb_buffer + 512;
    jtag_commands (buffer, record->udata);
}


void jtag_init_usb (void)
{
    // No 0-size-frame.
    qh_init (JTAG_EP_OUT, 0x22000000);
    qh_init (JTAG_EP_IN, 0x22000000);
    *ENDPTCTRL4 = 0x008c008c;

    schedule_buffer (JTAG_EP_OUT, usb_buffer, 512, jtag_rx_complete);
    schedule_buffer (JTAG_EP_OUT, usb_buffer + 512, 512, jtag_rx_complete);
}
