
#include "registers.h"

#include <stdbool.h>

#define MAX_ITER 64
#define INT_BITS 4
#define FRAC_BITS (32 - 4)
#define BOUND (2 << FRAC_BITS)

typedef unsigned char pixel_t;

// Ugh.  gcc sucks.
union ll_maker {
    struct {
        unsigned lo;
        unsigned hi;
    };
    long long ll;
};

static int sq_sum_r(int z_r, int z_i, long long rr)
{
    rr += z_r * (long long) z_r;
    rr += z_i * (long long) -z_i;
    int r = (int) (rr >> FRAC_BITS);
    r += (rr & (1 << (FRAC_BITS - 1))) != 0;
    return r;
}

static int sq_sum_i(int z_r, int z_i, long long ii)
{
    // Instead of computing 2*x*y we include the 2* in the shifts.
    ii += z_r * (long long) z_i;
    int r = (int) (ii >> (FRAC_BITS - 1));
    r += (ii & (1 << (FRAC_BITS - 2))) != 0;
    return r;
}

typedef struct draw_context_t {
    int scale;                          // Multiply integer to give fixed point.
    int left;                           // Fixed-point.
    int top;                            // Fixed-point.
} draw_context_t;


static pixel_t evaluate(int c_r, int c_i, const draw_context_t * ctxt)
{
    union ll_maker u_r = { .lo = c_r << FRAC_BITS, .hi = c_r >> INT_BITS };
    long long cc_r = u_r.ll;

    // Instead of computing 2*x*y we include the 2* in the shifts.
    union ll_maker u_i = { .lo = c_i << (FRAC_BITS - 1),
                           .hi = (c_i >> (INT_BITS + 1)) };
    long long cc_i = u_i.ll;
    int z_i = 0;
    int z_r = 0;
    int i;
    for (i = 0; i < 1024; ++i) {
        int rr = sq_sum_r(z_r, z_i, cc_r);
        int ii = sq_sum_i(z_r, z_i, cc_i);
        if (rr <= -BOUND || rr >= BOUND || ii <= -BOUND || ii >= BOUND)
            return i & 63;
        z_r = rr;
        z_i = ii;
    }
    return 64;
}


// Return fixed point x co-ordinate of pixel address.
static int x_fixed(const pixel_t * origin, const draw_context_t * ctxt)
{
    int x = ((unsigned) origin) & 1023;  // Pixels
    return x * ctxt->scale + ctxt->left;     // Convert to fixed-point.
}

// Return fixed point y co-ordinate of pixel address.
static int y_fixed(pixel_t * origin, const draw_context_t * ctxt)
{
    int y = ((unsigned) origin >> 10) & 1023; // Pixels
    return y * ctxt->scale + ctxt->top;       // Convert to fixed-point.
}


// Draw along horizontal line.  The pixels are stored into both the
// origin buffer and the temp buffer.
static void draw_hline(pixel_t * origin, pixel_t * temp,
                       int length, const draw_context_t * ctxt)
{
    int x = x_fixed(origin, ctxt);
    int y = y_fixed(origin, ctxt);
    for (int i = 0; i < length; ++i, x += ctxt->scale)
        temp[i] = origin[i] = evaluate(x, y, ctxt);
}


// Draw along vertical line.  The pixels are stored into the temp buffer
// and not the origin buffer.
static void draw_vline(pixel_t * origin, pixel_t * temp,
                       int length, const draw_context_t * restrict ctxt)
{
    int x = x_fixed(origin, ctxt);
    int y = y_fixed(origin, ctxt);
    for (int i = 0; i < length; ++i, y += ctxt->scale)
        temp[i] = evaluate(x, y, ctxt);
}


static bool is_constant(int width, int height,
                        const pixel_t * top, const pixel_t * bot,
                        const pixel_t * left, const pixel_t * right)
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
static void draw(pixel_t * origin, int width, int height,
                 draw_context_t * ctxt,
                 const pixel_t * top, const pixel_t * bot,
                 const pixel_t * left, const pixel_t * right)
{
    if (is_constant(width, height, top, bot, left, right)
        && width <= 256 && height <= 256) {
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


// Width and height are inclusive.
static void draw_rect(pixel_t * origin, int width, int height,
                      draw_context_t * ctxt)
{
    for (int y = 0; y <= height; ++y)
        for (int x = 0; x <= width; ++x)
            origin[y * 1024 + x] = 65;

    pixel_t edges[width * 2 + height * 2 + 4];
    pixel_t * top = edges;
    draw_hline(origin, top, width + 1, ctxt);
    if (height == 0)
        return;

    pixel_t * bot = edges + width + 1;
    draw_hline(origin + 1024 * height, bot, width + 1, ctxt);

    pixel_t * right = bot + width + 1;
    draw_vline(origin + width, right, height + 1, ctxt);
    // It's up to us to plot those...
    for (int i = 1; i < height; ++i)
        origin[i * 1024 + width] = right[i];

    if (width == 0)
        return;

    pixel_t * left = right + height + 1;
    draw_vline(origin, left, height + 1, ctxt);

    if (height > 1)
        draw(origin, width, height, ctxt, top, bot, left, right);
}


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


static bool key_stroke(draw_context_t * restrict ctxt, bool * scale_change,
                       int * deltax, int * deltay)
{
    // We keep the center point (pixel 512,512) within -2.0 ... 2.0
    // and the total width no more than 4.
    int c = getchar();
    switch (c) {
    case 'A':                   // Up.
        if (ctxt->top + 513 * ctxt->scale >= (2 << FRAC_BITS))
            break;
        --*deltay;
        ctxt->top += ctxt->scale;
        break;
    case 'B':                   // Down.
        if (ctxt->top + 511 * ctxt->scale < (-2 << FRAC_BITS))
            break;
        ++*deltay;
        ctxt->top -= ctxt->scale;
        break;
    case 'C':                   // Right.
        if (ctxt->left + 511 * ctxt->scale < (-2 << FRAC_BITS))
            break;
        ++*deltax;
        ctxt->left -= ctxt->scale;
        break;
    case 'D':                   // Left.
        if (ctxt->left + 513 * ctxt->scale > (2 << FRAC_BITS))
            break;
        --*deltax;
        ctxt->left += ctxt->scale;
        break;;
    case '5': { // Page up.
        // Zoom-in keeping (512,512) in the same place...
        int scale = ctxt->scale >> 1;
        if (scale < 1)
            break;
        *scale_change = true;
        ctxt->left += 512 * (ctxt->scale - scale);
        ctxt->top += 512 * (ctxt->scale - scale);
        ctxt->scale = scale;
        break;
    }
    case '6': {                 // Page down.
        int scale = ctxt->scale << 1;
        if (scale > (4 << (FRAC_BITS - 10)))
            break;
        int delta = 512 * (ctxt->scale - scale);
        *scale_change = true;
        ctxt->scale = scale;
        ctxt->left += delta;
        ctxt->top += delta;
        break;
    }
    case '\n':
        return true;
    }
    return false;
}


// Combine 3 bytes...
static unsigned rgb(unsigned r, unsigned g, unsigned b)
{
    return (r >> 3) | (g >> 3) * 32 | (b >> 3) * 1024
        | ((g & 4) ? 32768 : 0);
}


static unsigned colour(unsigned i)
{
    if (i < 32)                         // red ... green.
        return rgb(255 - i * 4, 128 + i * 4, 128);
    else                                // green ... blue.
        return rgb(128, 255 - (i - 32) * 4, 128 + (i - 32) * 4);
}


unsigned __section(".start") __attribute__((externally_visible)) mandel(void)
{
    *(volatile unsigned *) 0x40051600 = 1; // RIT clock enable.

    //unsigned start_clocks = * (volatile unsigned *) 0x400c000c;
    draw_context_t ctxt;

    for (unsigned i = 0; i < MAX_ITER/2; ++i)
        // 0 -> reg, 32 -> green, 63 -> blue, 64 -> black.
        LCD_PAL[i] = colour(2*i) + colour(2*i+1) * 65536;

    LCD_PAL[MAX_ITER/2] = 0xffff0000;

    LCD->ctrl = 0x00010827;             // 8bpp.

    ctxt.left = -2 << FRAC_BITS;
    ctxt.top = -2 << FRAC_BITS;
    ctxt.scale = 1 << (FRAC_BITS - 8);
    pixel_t * origin = (pixel_t *) 0x60000000;
    LCD->upbase = origin;
    bool scale_change = true;           // Force it first time round.
    while (true) {
        int deltax = 0;
        int deltay = 0;
        while (peekchar_nb() >= 0 || (!scale_change && !deltax && !deltay))
            if (key_stroke(&ctxt, &scale_change, &deltax, &deltay))
                return 0;

        if (scale_change
            || deltax > 512 || deltax < -512 || deltay > 512 || deltay < -512) {
            // Just redraw....
            draw_rect(origin, 1023, 1023, &ctxt);
            scale_change = false;
            continue;
        }

        // Shuffle data and repaint the damage areas.
        int delta = deltax + 1024 * deltay;
        if (delta > 0)
            for (int i = 1024 * 1024; i >= delta; --i)
                origin[i] = origin[i - delta];
        else
            for (int i = -delta; i < 1024 * 1024; ++i)
                origin[i + delta] = origin[i];

        if (deltay > 0)                 // Do any top damage area.
            draw_rect(origin, 1023, deltay - 1, &ctxt);
        if (deltay < 0)                 // Bottom.
            draw_rect(origin + 1024 * (1024 + deltay),
                      1023, -deltay - 1, &ctxt);
        if (deltax > 0)                 // Left.
            draw_rect(origin, deltax - 1, 1023, &ctxt);
        if (deltax < 0)                 // Right.
            draw_rect(origin + 1024 + deltax, -deltax - 1, 1023, &ctxt);

        //unsigned end_clocks = * (volatile unsigned *) 0x400c000c;
        //return end_clocks - start_clocks;
    }
}
