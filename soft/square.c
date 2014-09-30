
#include "lcd.h"
#include "monkey.h"
#include "registers.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct sq_context_t {
    int x, y;
    unsigned colour, Lcolour;
    unsigned next_colour;
    pixel_t * frame_buffer;
} sq_context_t;

static pixel_t colour(unsigned i)
{
    // Pixel layout msb to lsb is BBBB BGGG GGGR RRRR.
    // We do the colours in 64 steps, running through 3 segments.
    // Each segment is divided into pieces running one colour up and another
    // down, the top 75%.
    unsigned segment = (i * 3) >> 6;
    unsigned part = ((i * 3) & 63) * 3; // 0 <= part < 191.
    unsigned r, g, b;
    switch (segment) {
    case 0:                             // Red to green.
        r = 255 - part;
        g = 64 + part;
        b = 64;
        break;
    case 1:                             // Green to blue.
        r = 64;
        g = 255 - part;
        b = 64 + part;
        break;
    case 2:                             // Blue to red.
        r = 64 + part;
        g = 64;
        b = 255 - part;
        break;
    default:
        __builtin_unreachable();
    }
    return (r >> 3) | (g >> 2) * 32 | (b >> 3) * 2048;
}


typedef struct block16_t {
    unsigned short offset;
    short move;
    unsigned short data[16];
} block16_t;


static const block16_t block8[4] = {
    // Down 8, offset = 1,3, move = 0,8
    { 1 + 3 * 1024, 8 * 1024, {
            0x003e, 0x006b, 0x00e1, 0x01e3, 0x0100, 0x0180, 0x01e0, 0x01a0,
            0x01e0, 0x0180, 0x0100, 0x01e1, 0x00e1, 0x006b, 0x003e, 0x0000 }},
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
    { 1, 1 + 1024, 1 + 2048, 2048 },    // Down.
    { -1024, -1024+1, -1024+2, 2 },     // Right.
    { -1, -1-1024, -1-2048, -2048 },    // Up.
    { 1024, 1024-1, 1024-2, -2 },       // Left.
};


static pixel_t * square_draw (pixel_t * start, int dir, unsigned L, unsigned c)
{
    if (L == 4)
        return draw8(start, block8 + dir, c);

    if (L == 1) {
        const short * p = block1[dir];
        *start = c;
        start[*p++] = c;
        start[*p++] = c;
        start[*p++] = c;
        return start + *p;
    }

    L >>= 1;
    start = square_draw (start, (dir + 1) & 3, L, c);
    start = square_draw (start, dir, L, c);
    start = square_draw (start, dir, L, c);
    start = square_draw (start, (dir - 1) & 3, L, c);
    return start;
}

// We keep coords and (3<<level) in range -(1<<30)..(1<<30), so the arithmetic
// does not overflow.
static sq_context_t * square_clip(sq_context_t * restrict c, unsigned dir,
                                  unsigned L)
{
    if (__builtin_expect(L == 0, 1)) {
        if (c->x >= 0 && c->x < 1024 && c->y >= 0 && c->y < 1024)
            c->frame_buffer[c->x + c->y * 1024] = c->colour;
        L = 1;
        goto move;
    }
    if (L == c->Lcolour)
        c->colour = colour(c->next_colour++);

    // Work out bounding box.
    int right, up, left, down;
    switch (dir) {
    case 0:                             // down
        down  = (3 * L) - 1;
        right = (2 * L) - 1;
        up    = L - 1;
        left  = (L - 1) >> 1;
        break;
    case 1:                             // right
        right = (3 * L) - 1;
        up    = (2 * L) - 1;
        left  = L - 1;
        down  = (L - 1) >> 1;
        break;
    case 2:                             // up
        up    = (3 * L) - 1;
        left  = (2 * L) - 1;
        down  = L - 1;
        right = (L - 1) >> 1;
        break;
    case 3:                             // left
        left  = (3 * L) - 1;
        down  = (2 * L) - 1;
        right = L - 1;
        up    = (L - 1) >> 1;
        break;
    default:
        __builtin_unreachable();
    }
    right = c->x + right;
    up    = c->y - up;
    left  = c->x - left;
    down  = c->y + down;
    if (L <= c->Lcolour) {
        if (right < 0 || up >= 1024 || left >= 1024 || down < 0) {
            L *= 2;
            goto move;
        }

        if (right < 1024 && up >= 0 && left >= 0 && down < 1024) {
            square_draw(c->frame_buffer + c->x + c->y * 1024,
                        dir, L, c->colour);
            L *= 2;
            goto move;
        }
    }

    L >>= 1;
    c = square_clip(c, (dir + 1) & 3, L);
    c = square_clip(c, dir, L);
    c = square_clip(c, dir, L);
    return square_clip(c, (dir - 1) & 3, L);
move:
    * (dir & 1 ? &c->x : &c->y) += dir & 2 ? -L : L;
    return c;
}


static void dma_fill (void * p, unsigned pattern, unsigned n)
{
    const volatile unsigned source = pattern;
    volatile gpdma_channel_t * channel = &GPDMA->channel[7];
    GPDMA->config = 1;                  // Duplicates monkey...
    channel->config = 0;
    channel->control = 0;
    channel->srcaddr = &source;
    channel->lli = NULL;

    do {
        channel->destaddr = p;

        unsigned amount = n;
        if (amount > 4094)
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
        p += amount * 4;

        __interrupt_disable();
        while (channel->config & 1)
            __interrupt_wait_go();

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
    c.next_colour = 0;
    c.frame_buffer = FRAME_BUFFER;
    square_clip(&c, 1, 256);
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
            c.next_colour = 0;
            c.frame_buffer = new_frame;
            square_clip(&c, 1, L);
            lcd_setframe_wait (new_frame);
            // Clear out the old one.  FIXME - record number of pixels so we can
            // make a smart decision?
            dma_fill(current_frame, 0x42084208, 524288);
            pixel_t * temp = current_frame;
            current_frame = new_frame;
            new_frame = temp;

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
