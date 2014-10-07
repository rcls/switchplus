
#include "registers.h"

#define MAX_ITER 64
#define INT_BITS 4
#define FRAC_BITS (32 - 4)
#define BOUND (2 << FRAC_BITS)

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

static int evaluate(int c_r, int c_i)
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
    return i;
}


typedef struct draw_context_t {
    unsigned scale;                     // Multiply integer to give fixed point.
    int left;                           // Fixed-point.
    int top;                            // Fixed-point.
    unsigned short col[MAX_ITER + 1];
} draw_context_t;


static int x_fixed(unsigned short * restrict origin,
                   const draw_context_t * restrict ctxt)
{
    int x = ((unsigned) origin >> 1) & 1023; // Pixels
    return x * ctxt->scale + ctxt->left;     // Convert to fixed-point.
}

static int y_fixed(unsigned short * restrict origin,
                   const draw_context_t * restrict ctxt)
{
    int y = ((unsigned) origin >> 11) & 1023; // Pixels
    return y * ctxt->scale + ctxt->top;       // Convert to fixed-point.
}


// Draw along horizontal line.
static void draw_hline(unsigned short * restrict origin,
                       int length,
                       const draw_context_t * restrict ctxt)
{
    int x = x_fixed(origin, ctxt);
    int y = y_fixed(origin, ctxt);
    for (int i = 0; i < length; ++i, x += ctxt->scale)
        origin[i] = ctxt->col[evaluate(x, y)];
}


// Draw along vetical line.
static void draw_vline(unsigned short * restrict origin,
                       int length, const draw_context_t * restrict ctxt)
{
    int x = x_fixed(origin, ctxt);
    int y = y_fixed(origin, ctxt);
    for (int i = 0; i < length; ++i, y += ctxt->scale)
        origin[i * 1024] = ctxt->col[evaluate(x, y)];
}


// The width and height are inclusive but the boundary is already drawn.
static void draw(unsigned short * restrict origin,
                 int width, int height,
                 const draw_context_t * restrict ctxt)
{
    // The boundary is already computed.
    unsigned short b = *origin;
    for (int i = 1; i <= width; ++i)
        if (origin[i] != b)
            goto split;

    for (int i = 0; i <= width; ++i)
        if (origin[height * 1024 + i] != b)
            goto split;

    for (int i = 1; i < height; ++i)
        if (origin[i * 1024] != b)
            goto split;

    for (int i = 1; i < height; ++i)
        if (origin[i * 1024 + width] != b)
            goto split;

    // Constant....
    for (int y = 1; y < height; ++y)
        for (int x = 1; x < width; ++x)
            origin[y * 1024 + x] = b;
    return;

split: ;
    if (width <= 3 || height <= 3) {
        // Not worth splitting, just compute.  The boundary is drawn so we start
        // at (1,1) not (0,0).
        int xbase = x_fixed(origin + 1025, ctxt);
        int yi = y_fixed(origin + 1025, ctxt);
        for (int y = 1; y < height; ++y, yi += ctxt->scale) {
            int xr = xbase;
            for (int x = 1; x < width; ++x, xr += ctxt->scale)
                origin[y * 1024 + x] = ctxt->col[evaluate(xr, yi)];
        }
        return;
    }

    if (width <= height) {
        // Split into two rectangles, one below the other.
        int midh = height >> 1;
        draw_hline(origin + midh * 1024 + 1, width - 1, ctxt);
        draw(origin, width, midh, ctxt);
        draw(origin + midh * 1024, width, height - midh, ctxt);
    }
    else {
        // Split into two rectangles, one right of the other.
        int midw = width >> 1;
        draw_vline(origin + 1024 + midw, height - 1, ctxt);
        draw(origin, midw, height, ctxt);
        draw(origin + midw, width - midw, height, ctxt);
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
#if 0
    unsigned short col[MAX_ITER + 1];
    for (unsigned i = 0; i < MAX_ITER; ++i) {
        // 0 -> reg, 32 -> green, 63 -> blue, 64 -> black.
        if (i < 32)
            col[i] = (32 - i) + 2 * i * 32 + 16 * 64 * 32;
        else
            col[i] = 16 + (128 - 2 * i) * 32 + (i - 32) * 64 * 32;
    }
    col[MAX_ITER] = 0;

    unsigned short * base = (unsigned short *) 0x60000000;
    LCD->upbase = base;
    for (int y = 0; y < 1024; ++y)
        for (int x = 0; x < 1024; ++x)
            base[y * 1024 + x] = col[
                evaluate((x - 512) << (FRAC_BITS - 8),
                         (y - 512) << (FRAC_BITS - 8))];
#endif
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
    unsigned short * origin = (unsigned short *) 0x60000000;
    LCD->upbase = origin;
    for (int i = 0; i != 1024 * 1024; ++i)
        origin[i] = 0xffff;
    // Draw the 4 boundaries...
    draw_hline(origin, 1024, &ctxt);
    draw_hline(origin + 1023 * 1024, 1024, &ctxt);
    draw_vline(origin + 1024, 1022, &ctxt);
    draw_vline(origin + 2047, 1022, &ctxt);
    draw(origin, 1023, 1023, &ctxt);

    unsigned end_clocks = * (volatile unsigned *) 0x400c000c;
    return end_clocks - start_clocks;
}
