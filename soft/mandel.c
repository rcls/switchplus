
#include "registers.h"

#include <stdbool.h>

#define MAX_ITER 64
#define INT_BITS 4
#define FRAC_BITS (32 - 4)
#define BOUND (2 << FRAC_BITS)

typedef unsigned short pixel_t;

// Ugh.  gcc sucks.
union ll_maker {
    struct {
        unsigned lo;
        unsigned hi;
    };
    long long ll;
};

static int sq_sum_r(int z_r, int z_i, int c_r)
{
    union ll_maker aa = { .lo = c_r << FRAC_BITS, .hi = c_r >> INT_BITS };
    long long a = aa.ll;
    a += z_r * (long long) z_r;
    a += z_i * (long long) -z_i;
    int r = (int) (a >> FRAC_BITS);
    r += (a & (1 << (FRAC_BITS - 1))) != 0;
    return r;
}

static int sq_sum_i(int z_r, int z_i, int c_i)
{
    // Instead of computing 2*x*y we include it in the shifts.
    union ll_maker aa = { .lo = c_i << (FRAC_BITS - 1),
                          .hi = (c_i >> (INT_BITS + 1)) };
    long long a = aa.ll;
    a += z_r * (long long) z_i;
    int r = (int) (a >> (FRAC_BITS - 1));
    r += (a & (1 << (FRAC_BITS - 2))) != 0;
    return r;
}

typedef struct draw_context_t {
    unsigned scale;                     // Multiply integer to give fixed point.
    int left;                           // Fixed-point.
    int top;                            // Fixed-point.
    pixel_t col[MAX_ITER + 1];
} draw_context_t;


static pixel_t evaluate(int c_r, int c_i,
                               const draw_context_t * restrict ctxt)
{
    int z_i = 0;
    int z_r = 0;
    int i;
    for (i = 0; i < MAX_ITER; ++i) {
        int rr = sq_sum_r(z_r, z_i, c_r);
        int ii = sq_sum_i(z_r, z_i, c_i);
        if (rr <= -BOUND || rr >= BOUND || ii <= -BOUND || ii >= BOUND)
            break;
        z_r = rr;
        z_i = ii;
    }
    return ctxt->col[i];
}


static int x_fixed(pixel_t * restrict origin,
                   const draw_context_t * restrict ctxt)
{
    int x = ((unsigned) origin >> 1) & 1023; // Pixels
    return x * ctxt->scale + ctxt->left;     // Convert to fixed-point.
}

static int y_fixed(pixel_t * restrict origin,
                   const draw_context_t * restrict ctxt)
{
    int y = ((unsigned) origin >> 11) & 1023; // Pixels
    return y * ctxt->scale + ctxt->top;       // Convert to fixed-point.
}


// Draw along horizontal line.  The pixels are stored into both the
// origin buffer and the temp buffer.
static void draw_hline(pixel_t * restrict origin,
                       pixel_t * restrict temp,
                       int length,
                       const draw_context_t * restrict ctxt)
{
    int x = x_fixed(origin, ctxt);
    int y = y_fixed(origin, ctxt);
    for (int i = 0; i < length; ++i, x += ctxt->scale)
        temp[i] = origin[i] = evaluate(x, y, ctxt);
}


// Draw along vertical line.  The pixels are stored into the temp buffer
// and not the origin buffer.
static void draw_vline(pixel_t * restrict origin,
                       pixel_t * restrict temp,
                       int length, const draw_context_t * restrict ctxt)
{
    int x = x_fixed(origin, ctxt);
    int y = y_fixed(origin, ctxt);
    for (int i = 0; i < length; ++i, y += ctxt->scale)
        temp[i] = evaluate(x, y, ctxt);
}


static bool is_constant(int width, int height,
                        const pixel_t * restrict top,
                        const pixel_t * restrict bot,
                        const pixel_t * restrict left,
                        const pixel_t * restrict right)
{
    pixel_t b = left[0];
    for (int i = 0; i <= width; ++i)
        if (top[i] != b || bot[i] != b)
            return false;

    for (int i = 1; i < height; ++i)
        if (left[i] != b || right[i] != b)
            return false;

    return true;
}

// The width and height are inclusive.  Top, bottom and right boundaries are
// drawn by the caller, we draw the left boundary.
static void draw(pixel_t * restrict origin,
                 int width, int height,
                 draw_context_t * restrict ctxt,
                 const pixel_t * restrict top,
                 const pixel_t * restrict bot,
                 const pixel_t * restrict left,
                 const pixel_t * restrict right)
{
    if (is_constant(width, height, top, bot, left, right)) {
        pixel_t b = left[0];
        for (int y = 1; y < height; ++y)
            for (int x = 0; x < width; ++x)
                origin[y * 1024 + x] = b;
        return;
    }

    if (width <= 3 || height <= 3) {
        // Not worth splitting, just compute.  The boundary is drawn so we start
        // at (1,1) not (0,0).
        int xbase = x_fixed(origin + 1025, ctxt);
        int yi = y_fixed(origin + 1025, ctxt);
        for (int y = 1; y < height; ++y, yi += ctxt->scale) {
            origin[y * 1024] = left[y];
            int xr = xbase;
            for (int x = 1; x < width; ++x, xr += ctxt->scale)
                origin[y * 1024 + x] = evaluate(xr, yi, ctxt);
        }
        return;
    }

    if (width <= height) {
        // Split into two rectangles, one below the other.
        int midh = height >> 1;
        pixel_t mid[width + 1];

        // The left pixel is our responsibility.
        mid[0] = left[midh];
        origin[midh * 1024] = mid[0];
        draw_hline(origin + midh * 1024 + 1, mid + 1, width - 1, ctxt);
        mid[width] = right[midh];

        draw(origin, width, midh, ctxt, top, mid, left, right);
        draw(origin + midh * 1024, width, height - midh, ctxt,
             mid, bot, left + midh, right + midh);
    }
    else {
        // Split into two rectangles, one right of the other.
        int midw = width >> 1;
        pixel_t mid[height + 1];

        mid[0] = top[midw];
        draw_vline(origin + 1024 + midw, mid + 1, height - 1, ctxt);
        mid[height] = bot[midw];

        draw(origin, midw, height, ctxt, top, bot, left, mid);
        draw(origin + midw, width - midw, height, ctxt,
             top + midw, bot + midw, mid, right);
    }
}


#if 0
static int getchar(void)
{
    typedef int getchar_t(void);
    void * const * except = SCB->vtor;
    getchar_t * f = except[63];
    return f();
}

static int peekchar_nb(void)
{
    typedef int pc_nb_t(void);
    void * const * except = SCB->vtor;
    pc_nb_t * f = except[62];
    return f();
}
#endif


unsigned __section(".start") __attribute__((externally_visible)) mandel(void)
{
    *(volatile unsigned *) 0x40051600 = 1;

    unsigned start_clocks = * (volatile unsigned *) 0x400c000c;
    draw_context_t ctxt;
    ctxt.left = -2 << FRAC_BITS;
    ctxt.top = -2 << FRAC_BITS;
    ctxt.scale = 1 << (FRAC_BITS - 8);
    for (unsigned i = 0; i < MAX_ITER; ++i) {
        // 0 -> reg, 32 -> green, 63 -> blue, 64 -> black.
        if (i < 32)
            ctxt.col[i] = (32 - i) + 2 * i * 32 + 16 * 64 * 32;
        else
            ctxt.col[i] = 16 + (128 - 2 * i) * 32 + (i - 32) * 64 * 32;
    }
    ctxt.col[MAX_ITER] = 0;
    pixel_t * origin = (pixel_t *) 0x60000000;
    LCD->upbase = origin;
    for (int i = 0; i != 1024 * 1024; ++i)
        origin[i] = 0xffff;
    // Draw the 4 boundaries...
    pixel_t top[1024];
    draw_hline(origin, top, 1024, &ctxt);

    pixel_t bottom[1024];
    draw_hline(origin + 1023 * 1024, bottom, 1024, &ctxt);

    pixel_t left[1024];
    draw_vline(origin, left, 1024, &ctxt);

    pixel_t right[1024];
    draw_vline(origin + 1023, right, 1024, &ctxt);

    for (int i = 0; i < 1024; ++i)
        origin[i * 1024 + 1023] = right[i];

    draw(origin, 1023, 1023, &ctxt, top, bottom, left, right);

    unsigned end_clocks = * (volatile unsigned *) 0x400c000c;
    return end_clocks - start_clocks;
}
