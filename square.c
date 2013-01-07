
#include "monkey.h"
#include "registers.h"

#include <stdbool.h>

#define FRAME_BUFFER ((unsigned short *) 0x60000000)

typedef unsigned short pixel_t;

typedef struct sq_context_t {
    int x, y;
    unsigned colour, Lcolour;
    const pixel_t * next_colour;
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


static pixel_t * square_draw1_right(pixel_t * start, int colour)
{
    start[0] = colour;
    start[-1024] = colour;
    start[-1024+1] = colour;
    start[-1024+2] = colour;
    return start+2;
}

static pixel_t * square_draw1_up(pixel_t * start, int colour)
{
    start[0] = colour;
    start[-1] = colour;
    start[-1-1024] = colour;
    start[-1-2048] = colour;
    return start - 2048;
}


static pixel_t * square_draw1_left(pixel_t * start, int colour)
{
    start[0] = colour;
    start[1024] = colour;
    start[1024-1] = colour;
    start[1024-2] = colour;
    return start - 2;
}


static pixel_t * square_draw1_down(pixel_t * start, int colour)
{
    start[0] = colour;
    start[1] = colour;
    start[1 + 1024] = colour;
    start[1 + 2048] = colour;
    return start + 2048;
}


static pixel_t * square_draw (pixel_t * start, int dir, unsigned L, unsigned c)
{
    if (L == 1) {
        if (dir & 2)
            if (dir & 1)
                return square_draw1_down(start, c); // 3 = down
            else
                return square_draw1_left(start, c); // 2 = left
        else
            if (dir & 1)
                return square_draw1_up(start, c); // 1 = up
            else
                return square_draw1_right(start, c); // 0 = right
    }
    else {
        L >>= 1;
        start = square_draw (start, dir + 1, L, c);
        start = square_draw (start, dir, L, c);
        start = square_draw (start, dir, L, c);
        start = square_draw (start, dir - 1, L, c);
        return start;
    }
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


static inline sq_context_t * plot (int x, int y, sq_context_t * c)
{
    if (x >= 0 && x < 1024 && y >= 0 && y < 1024)
        FRAME_BUFFER[x + y * 1024] = c->colour;
    return c;
}


#define SQUARE_BLK(MAIN,FIRST,LAST,DIR)                                 \
    if (L == c->Lcolour)                                                \
        c->colour = *c->next_colour++;                                  \
                                                                        \
    if (L <= c->Lcolour && fits (c->x, c->y, B_##MAIN))                 \
        square_draw(FRAME_BUFFER + c->x + c->y * 1024, DIR, L, c->colour); \
                                                                        \
    else if (L > c->Lcolour || !off (c->x, c->y, B_##MAIN)) {           \
        L >>= 1;                                                        \
        c = square_##FIRST(c, L);                                       \
        c = square_##MAIN(c, L);                                        \
        c = square_##MAIN(c, L);                                        \
       return square_##LAST(c, L);                                      \
    }


static sq_context_t * square_right (sq_context_t * restrict c, unsigned L)
{
    if (L == 0)
        return plot(c->x++, c->y, c);

    SQUARE_BLK(right, up, down, 0);

    c->x += 2 * L;
    return c;
}


static sq_context_t * square_up (sq_context_t * restrict c, unsigned L)
{
    if (L == 0)
        return plot(c->x, c->y--, c);

    SQUARE_BLK(up, left, right, 1);

    c->y -= 2 * L;
    return c;
}


static sq_context_t * square_left (sq_context_t * restrict c, unsigned L)
{
    if (L == 0)
        return plot(c->x--, c->y, c);

    SQUARE_BLK(left, down, up, 2);

    c->x -= 2 * L;
    return c;
}


static sq_context_t * square_down (sq_context_t * restrict c, unsigned L)
{
    if (L == 0)
        return plot(c->x, c->y++, c);

    SQUARE_BLK(down, right, left, 3);

    c->y += 2 * L;
    return c;
}


void square_draw9 (void)
{
    verbose("Drawing squaral...");
    __memory_barrier();
    pixel_t * origin = (pixel_t *) 0x60000000;
    for (int i = 0; i != 1048576; ++i)
        origin[i] = 0x4208;
    sq_context_t c;
    c.x = 256;
    c.y = 768;
    c.Lcolour = 256 / 8;
    c.next_colour = colours;
    square_right(&c, 256);
    __memory_barrier();
    verbose(" done.\n");
}


void square_interact (void)
{
    // Clear out a frame.
    __memory_barrier();
    for (int i = 0; i != 1048576; ++i)
        FRAME_BUFFER[i] = 0x4208;
    __memory_barrier();

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
            __memory_barrier();
            // Clear out the previous.
            sq_context_t c;
            c.x = lastX;
            c.y = lastY;
            c.Lcolour = 0xffffffff;
            c.colour = 0x4208;
            c.next_colour = colours;
            square_right(&c, lastL);
            // Now draw the new one.
            c.x = X;
            c.y = Y;

            c.Lcolour = L >> 3;
            c.next_colour = colours;
            square_right(&c, L);
            lastX = X;
            lastY = Y;
            lastL = L;
            __memory_barrier();
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
            if (L > 16) {
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
