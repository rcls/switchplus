
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SPAN 32

static bool * draw(bool * s, int dir, int L)
{
    if (L > 1) {
        L >>= 1;
        s = draw(s, dir+1, L);
        s = draw(s, dir  , L);
        s = draw(s, dir  , L);
        s = draw(s, dir-1, L);
        return s;
    }

    *s = 1;
    switch (dir & 3) {
    case 0:                             // Down
        return s+SPAN;
    case 1:                             // Right.
        return s+1;
    case 2:                             // Up
        return s-SPAN;
    case 3:                             // Left
    default:
        return s-1;
    }
}


static unsigned grab(const bool * s, int n)
{
    unsigned v = 0;
    for (int i = 0; i < n; ++i)
        if (*s++)
            v += 1 << i;
    return v;
}


static void output(const bool * s, int n, int digits)
{
    for (int i = 0; i < n; ++i) {
        printf("0x%0*x, ", digits, grab(s + SPAN * i, n));
        if (i % 8 == 7)
            printf ("\n");
    }
    if (n % 8)
        printf ("\n");
}


static bool row_inhabited(const bool * s, int row)
{
    s += row * SPAN;
    for (int i = 0; i < SPAN; ++i)
        if (*s++)
            return true;
    return false;
}


static bool col_inhabited(const bool * s, int col)
{
    s += col;
    for (int i = 0; i < SPAN; ++i, s += SPAN)
        if (*s)
            return true;
    return false;
}


static void squaral(int dir, int L, int digits)
{
    bool buffer[SPAN * SPAN];
    // bool * buffer = malloc(SPAN * SPAN);
    memset(buffer, 0, SPAN * SPAN);
    bool * start = buffer + (SPAN + 1) * SPAN / 2;
    bool * finish = draw (buffer + (SPAN + 1) * SPAN / 2, dir, L);
    int top = 0;
    for (; !row_inhabited (buffer, top); ++top);
    int left = 0;
    for (; !col_inhabited (buffer, left); ++left);

    static const char * const dirs[] = { "Down", "Right", "Up", "Left" };
    printf ("// %s %d, offset = %d,%d, move = %ld,%ld\n",
            dirs[dir], L, SPAN / 2 - left, SPAN / 2 - top,
            (finish - start) % SPAN, (finish - start) / SPAN);
    output (buffer + top * SPAN + left, L * 2, digits);
}


int main()
{
    squaral(0, 4, 2);
    squaral(1, 4, 2);
    squaral(2, 4, 2);
    squaral(3, 4, 2);

    squaral(0, 8, 4);
    squaral(1, 8, 4);
    squaral(2, 8, 4);
    squaral(3, 8, 4);
    return 0;
}
