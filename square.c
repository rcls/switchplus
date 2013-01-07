
#include "monkey.h"
#include "registers.h"

// Pixel layout msb to lsb is BBBB BGGG GGGR RRRR.
static const unsigned short colours[] = {
    0x421f, 0x425e, 0x429d, 0x42dc, 0x431b, 0x435a, 0x4399, 0x43d8,
    0x4456, 0x4495, 0x44d4, 0x4513, 0x4552, 0x4591, 0x45d0, 0x460f,
    0x468d, 0x46cc, 0x470b, 0x474a, 0x4789, 0x47c8, 0x47c8, 0x4f88,
    0x5f08, 0x66c8, 0x6e88, 0x7648, 0x7e08, 0x85c8, 0x8d88, 0x9548,
    0xa4c8, 0xac88, 0xb448, 0xbc08, 0xc3c8, 0xcb88, 0xd348, 0xdb08,
    0xea88, 0xf248, 0xfa08, 0xfa08, 0xf209, 0xea0a, 0xe20b, 0xda0c,
    0xca0e, 0xc20f, 0xba10, 0xb211, 0xaa12, 0xa213, 0x9a14, 0x9215,
    0x8217, 0x7a18, 0x7219, 0x6a1a, 0x621b, 0x5a1c, 0x521d, 0x4a1e,
};


static unsigned short * square_draw1_right(unsigned short * start, int colour)
{
    start[0] = colour;
    start[-1024] = colour;
    start[-1024+1] = colour;
    start[-1024+2] = colour;
    return start+2;
}

static unsigned short * square_draw1_up(unsigned short * start, int colour)
{
    start[0] = colour;
    start[-1] = colour;
    start[-1-1024] = colour;
    start[-1-2048] = colour;
    return start - 2048;
}


static unsigned short * square_draw1_left(unsigned short * start, int colour)
{
    start[0] = colour;
    start[1024] = colour;
    start[1024-1] = colour;
    start[1024-2] = colour;
    return start - 2;
}


static unsigned short * square_draw1_down(unsigned short * start, int colour)
{
    start[0] = colour;
    start[1] = colour;
    start[1 + 1024] = colour;
    start[1 + 2048] = colour;
    return start + 2048;
}


static unsigned short * square_draw (unsigned short * start, int dir,
                                     int level, int * restrict count)
{
    if (--level == 0) {
        int colour = colours[*count];
        if (dir & 2)
            if (dir & 1)
                return square_draw1_down(start, colour); // 3 = down
            else
                return square_draw1_left(start, colour); // 2 = left
        else
            if (dir & 1)
                return square_draw1_up(start, colour); // 1 = up
            else
                return square_draw1_right(start, colour); // 0 = right
    }
    else {
        if (level == 9 - 4)
            ++*count;
        start = square_draw (start, dir + 1, level, count);
        start = square_draw (start, dir, level, count);
        start = square_draw (start, dir, level, count);
        start = square_draw (start, dir - 1, level, count);
        return start;
    }
}


void square_draw9 (void)
{
    verbose("Drawing squaral...");
    __memory_barrier();
    unsigned short * origin = (unsigned short *) 0x60000000;
    for (int i = 0; i != 1048576; ++i)
        origin[i] = 0x4208;
    int count = -1;
    square_draw(origin + 768 * 1024 + 256, 0, 9, &count);
    __memory_barrier();
    verbose(" done.\n");
    if (count != 63)
        printf("Count = %d\n", count);
}
