
#include "lcd.h"
#include "monkey.h"
#include "registers.h"

#include <stdbool.h>
#include <stddef.h>

typedef unsigned short pixel_t;

#define FRAME_BUFFER ((pixel_t *) 0x60000000)

typedef struct sq_context_t {
    int x, y;
    unsigned colour, Lcolour;
    const pixel_t * next_colour;
    pixel_t * frame_buffer;
} sq_context_t;

static sq_context_t * square_right(sq_context_t * restrict c, unsigned L);
static sq_context_t * square_up   (sq_context_t * restrict c, unsigned L);
static sq_context_t * square_left (sq_context_t * restrict c, unsigned L);
static sq_context_t * square_down (sq_context_t * restrict c, unsigned L);

// Pixel layout msb to lsb is BBBB BGGG GGGR RRRR.
static const pixel_t colours[] = {
    0x421f, 0x425e, 0x429d, 0x42dc, 0x431b, 0x435a, 0x4399, 0x43d8,
    0x4456, 0x4495, 0x44d4, 0x4513, 0x4552, 0x4591, 0x45d0, 0x460f,
    0x468d, 0x46cc, 0x470b, 0x474a, 0x4789, 0x47c8, 0x47c8, 0x4f88,
    0x5f08, 0x66c8, 0x6e88, 0x7648, 0x7e08, 0x85c8, 0x8d88, 0x9548,
    0xa4c8, 0xac88, 0xb448, 0xbc08, 0xc3c8, 0xcb88, 0xd348, 0xdb08,
    0xea88, 0xf248, 0xfa08, 0xfa08, 0xf209, 0xea0a, 0xe20b, 0xda0c,
    0xca0e, 0xc20f, 0xba10, 0xb211, 0xaa12, 0xa213, 0x9a14, 0x9215,
    0x8217, 0x7a18, 0x7219, 0x6a1a, 0x621b, 0x5a1c, 0x521d, 0x4a1e,
};


typedef struct block16_t {
    unsigned short offset;
    short move;
    unsigned short data[16];
} block16_t;


typedef struct block8_t {
    unsigned short offset;
    short move;
    unsigned char data[8];
} block8_t;


static const block16_t block8[4] = {
    // Right 8, offset = 3,7, move = 8,0
    { 3 + 7 * 1024, 8, {
            0x0ff8, 0x1bec, 0x394e, 0x79cf, 0x4001, 0x6003, 0x4001, 0x600b,
            0x380e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }},
    // Up 8, offset = 7,11, move = 0,-8
    { 7 + 11 * 1024, -8 * 1024, {
            0x00f8, 0x01ac, 0x010e, 0x010f, 0x0001, 0x0003, 0x000f, 0x000b,
            0x000f, 0x0003, 0x0001, 0x018f, 0x010e, 0x01ac, 0x00f8, 0x0000 }},
    // Left 8, offset = 11,1, move = -8,0
    { 11 + 1024, -8, {
            0x380e, 0x6803, 0x4001, 0x6003, 0x4001, 0x79cf, 0x394e, 0x1bec,
            0x0ff8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }},
    // Down 8, offset = 1,3, move = 0,8
    { 1 + 3 * 1024, 8 * 1024, {
            0x003e, 0x006b, 0x00e1, 0x01e3, 0x0100, 0x0180, 0x01e0, 0x01a0,
            0x01e0, 0x0180, 0x0100, 0x01e1, 0x00e1, 0x006b, 0x003e, 0x0000 }},
};


static pixel_t * draw8 (pixel_t * s, const block16_t * b, int colour)
{
    pixel_t * r = s + b->move;
    s -= b->offset;
    const unsigned short * data = b->data;
    unsigned bits = *data++;
    do {
        pixel_t * p = s;
        do {
            if (bits & 15) {
                if (bits & 1)
                    p[0] = colour;
                if (bits & 2)
                    p[1] = colour;
                if (bits & 4)
                    p[2] = colour;
                if (bits & 8)
                    p[3] = colour;
            }
            p += 4;
            bits >>= 4;
        }
        while (bits);
        s += 1024;
        bits = *data++;
    }
    while (bits);
    return r;
}


static const short block1[4][4] = {
    { -1024, -1024+1, -1024+2, 2 },
    { -1, -1-1024, -1-2048, -2048 },
    { 1024, 1024-1, 1024-2, -2 },
    { 1, 1 + 1024, 1 + 2048, 2048 }
};


static pixel_t * square_draw (pixel_t * start, int dir, unsigned L, unsigned c)
{
    if (L == 4)
        return draw8(start, block8 + (dir & 3), c);

    if (L == 1) {
        const short * p = block1[dir & 3];
        *start = c;
        start[*p++] = c;
        start[*p++] = c;
        start[*p++] = c;
        return start + *p;
    }

    L >>= 1;
    start = square_draw (start, dir + 1, L, c);
    start = square_draw (start, dir, L, c);
    start = square_draw (start, dir, L, c);
    start = square_draw (start, dir - 1, L, c);
    return start;
}


// We keep coords and (3<<level) in range -(1<<30)..(1<<30), so the arithmetic
// does not overflow.
static inline bool fits (int x, int y, int right, int up, int left, int down)
{
    return x - left >= 0 && x + right < 1024 && y - up >= 0 && y + down < 1024;
}


static inline bool off (int x, int y, int right, int up, int left, int down)
{
    return x - left >= 1024 || x + right < 0 || y - up >= 1024 || y + down < 0;
}


#define B_right (3 * L) - 1 , (2 * L) - 1 , L - 1       , (L - 1) >> 1
#define B_up    (L - 1) >> 1, (3 * L) - 1 , (2 * L) - 1 , L - 1
#define B_left  L - 1       , (L - 1) >> 1, (3 * L) - 1 , (2 * L) - 1
#define B_down  (2 * L) - 1 , L - 1       , (L - 1) >> 1, (3 * L) - 1

#define M_right(c,L) (c)->x += L
#define M_up(c,L)    (c)->y -= L
#define M_left(c,L)  (c)->x -= L
#define M_down(c,L)  (c)->y += L

#define SQUARE_BLK(MAIN,FIRST,LAST,DIR)                                 \
    if (L == 0) {                                                       \
        if (c->x >= 0 && c->x < 1024 && c->y >= 0 && c->y < 1024)       \
            c->frame_buffer[c->x + c->y * 1024] = c->colour;            \
        M_##MAIN(c,1);                                                  \
        return c;                                                       \
    }                                                                   \
    if (L == c->Lcolour)                                                \
        c->colour = *c->next_colour++;                                  \
                                                                        \
    if (L <= c->Lcolour && fits (c->x, c->y, B_##MAIN))                 \
        square_draw(c->frame_buffer + c->x + c->y * 1024,               \
                    DIR, L, c->colour);                                 \
                                                                        \
    else if (L > c->Lcolour || !off (c->x, c->y, B_##MAIN)) {           \
        L >>= 1;                                                        \
        c = square_##FIRST(c, L);                                       \
        c = square_##MAIN(c, L);                                        \
        c = square_##MAIN(c, L);                                        \
        return square_##LAST(c, L);                                     \
    }                                                                   \
                                                                        \
    M_##MAIN(c, 2*L);                                                   \
    return c;

static sq_context_t * square_right (sq_context_t * restrict c, unsigned L)
{
    SQUARE_BLK(right, up, down, 0);
}


static sq_context_t * square_up (sq_context_t * restrict c, unsigned L)
{
    SQUARE_BLK(up, left, right, 1);
}


static sq_context_t * square_left (sq_context_t * restrict c, unsigned L)
{
    SQUARE_BLK(left, down, up, 2);
}


static sq_context_t * square_down (sq_context_t * restrict c, unsigned L)
{
    SQUARE_BLK(down, right, left, 3);
}


void gpdma_interrupt (void)
{
    GPDMA->inttcclear = GPDMA->inttcstat;
    GPDMA->interrclr = GPDMA->interrstat;
}


static void dma_fill (void * p, unsigned pattern, unsigned n)
{
    * (v32 *) 0x40051440 = 1;           // Enabled clock.

    const volatile unsigned source = pattern;
    volatile gpdma_channel_t * channel = &GPDMA->channel[7];
    GPDMA->config = 1;
    channel->config = 0;
    channel->control = 0;
    channel->srcaddr = &source;
    channel->lli = NULL;

    do {
        channel->destaddr = p;

        unsigned amount = n;
        if (amount >= 4094)
            amount = 4094;
        // 2 : src & dst width = 32bits.
        // 4 : src burst-size = 32 transfers.
        // 0 : dst burst-size = 1 transfer.
        // Destination increment.  Enable terminal count interrupt.
        channel->control = (1 << 31) + (1 << 27)
            + (2 << 21) + (2 << 18) + (0 << 15) + (4 << 12) + amount;

        // Now kick it off, unmasking interrupts.
        channel->config = 0xc001;

        n -= amount;
        p = (void*) (amount * 4 + (unsigned) p);

        __interrupt_disable();
        while (channel->config & 1) {
            __interrupt_wait();
            __interrupt_enable();
            __interrupt_disable();
        }
        __interrupt_enable();
    }
    while (n);
}


void square_draw9 (void)
{
    verbose("Drawing squaral...");
    dma_fill (FRAME_BUFFER, 0x42084208, 524288);
    sq_context_t c;
    c.x = 256;
    c.y = 768;
    c.Lcolour = 256 / 8;
    c.next_colour = colours;
    c.frame_buffer = FRAME_BUFFER;
    square_right(&c, 256);
    __memory_barrier();
    verbose(" done.\n");
}


void square_interact (void)
{
    // Clear out two frames.
    pixel_t * current_frame = FRAME_BUFFER;
    pixel_t * new_frame = FRAME_BUFFER + 1048576;
    dma_fill (FRAME_BUFFER, 0x42084208, 1048576);
    lcd_setframe_wait (current_frame);

    int lastX = -8;
    int lastY = -8;
    int lastL = 1;

    unsigned fracX = 0;
    unsigned fracY = 0;
    int X = 256;
    int Y = 768;
    int L = 256;

    while (true) {
        if (peekchar_nb() < 0 && (lastX != X || lastY != Y || lastL != L)) {
            sq_context_t c;
            // Draw the new frame.
            c.x = X;
            c.y = Y;
            c.Lcolour = L >> 3;
            c.next_colour = colours;
            c.frame_buffer = new_frame;
            square_right(&c, L);
            lcd_setframe_wait (new_frame);
            // Clear out the old one.  FIXME - record number of pixels so we can
            // make a smart decision?
            if (false) {
                c.x = lastX;
                c.y = lastY;
                c.Lcolour = 0xffffffff;
                c.colour = 0x4208;
                c.next_colour = colours;
                c.frame_buffer = current_frame;
                current_frame = new_frame;
                new_frame = c.frame_buffer;
                square_right(&c, lastL);
            }
            else {
                dma_fill(current_frame, 0x42084208, 524288);
                pixel_t * temp = current_frame;
                current_frame = new_frame;
                new_frame = temp;
            }
            lastX = X;
            lastY = Y;
            lastL = L;
        }
        switch (getchar()) {
        case '\n':
        case '\r':
            return;
        case 'A':
            if (Y > -0x40000000)
                --Y;
            break;
        case 'B':
            if (Y + 1 < 0x40000000)
                ++Y;
            break;
        case 'C':
            if (X + 1 < 0x40000000)
                ++X;
            break;
        case 'D':
            if (X > -0x40000000)
                --X;
            break;
        case '5':
            if (L < 0x10000000 &&
                X >= -0x1fffff00 && X < 0x20000100 &&
                Y >= -0x1fffff00 && Y < 0x20000100) {
                L *= 2;
                X = X * 2 - 512 + (fracX & 1);
                Y = Y * 2 - 512 + (fracY & 1);
                fracX >>= 1;
                fracY >>= 1;
            }
            break;
        case '6':
            if (L > 8) {
                L >>= 1;
                fracX = fracX * 2 + (X & 1);
                fracY = fracY * 2 + (Y & 1);
                X = (X >> 1) + 256;
                Y = (Y >> 1) + 256;
            }
            break;
        }
    }
}
