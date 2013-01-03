
#include "characters.h"
#include "freq.h"
#include "lcd.h"
#include "monkey.h"
#include "registers.h"
#include "sdram.h"

#define FRAMEBUFFER ((unsigned short *) 0x60000000)
#define TEXT_LINES 64

static inline unsigned pll_np_div (unsigned pdec, unsigned ndec)
{
    return pdec + (ndec << 12);
}

void lcd_init (void)
{
    // Setup the PLL0AUDIO to give 52.25MHz off 50MHz ethernet clock.
    // ndec=5, mdec=32426, pdec=5
    // selr=0, seli=28, selp=14
    // pllfract = 26.125,0x0d1000
    // pre-divider=5, feedback div=26, post-div=5
    // fcco=522.5MHz, fout=52.25MHz.
    *PLL0AUDIO_CTRL = 0x03001811;
    *PLL0AUDIO_MDIV = 13107;
    *PLL0AUDIO_NP_DIV = pll_np_div(66, 122);
    *PLL0AUDIO_FRAC = 0x17eb85;

    *PLL0AUDIO_CTRL = 0x03001810;
    puts ("LCD lock wait:");
    while (!(*PLL0AUDIO_STAT & 1));
    puts (" done\n");

    // The lcd clock is outclk11.  PLL0AUDIO is clock source 8.
    *BASE_LCD_CLK = 0x08000800;

    // Reset the lcd.
    RESET_CTRL[0] = 1 << 16;
    while (!(RESET_ACTIVE_STATUS[0] & (1 << 16)));

    // Reset the SDRAM.
    meminit (cpu_frequency (1));

    // 1024x1024 59.90 Hz (CVT) hsync: 63.13 kHz; pclk: 74.75 MHz
    // Modeline "1024x1024R"   74.75  1024 1072 1104 1184  1024 1027 1037 1054
    // +hsync -vsync
    // horizontal 1024 48 32 80
    // vertical 1024 3 10 17

    *LCD_TIMH = (79 << 24) + (47 << 8) + (31 << 16) + 0xfc;
    *LCD_TIMV = (16 << 24) + (2 << 16) + (9 << 10) + 1023;
    *LCD_POL = 0x07ff3020;
    *LCD_UPBASE = (unsigned) FRAMEBUFFER;           // SDRAM.
    *LCD_LPBASE = (unsigned) FRAMEBUFFER;
    *LCD_CTRL = 0x2c;                   // TFT, 16bpp, disabled.

    // Setup pins.
    static const unsigned pins[] = {
#define pin_out(a,b,f) (a * 32 * 65536 + b * 65536 + 0x20 + f)

        pin_out(12,0,4),                // D4  LCD_DCLK
        pin_out(4,6,2),                 // C1  LCD_ENAB/LCDM
        pin_out(4,5,2),                 // D2  LCD_FP
        //pin_out(,,), // B16 LCD_LE
        pin_out(7,6,3),                 // C7  LCD_LP
        //pin_out(,,), // B6  LCD_PWR
        pin_out(4,1,2),                 // A1  LCD_VD0
        pin_out(4,4,2),                 // B1  LCD_VD1
        pin_out(4,3,2),                 // C2  LCD_VD2
        pin_out(4,2,2),                 // D3  LCD_VD3
        pin_out(8,7,3),                 // K1  LCD_VD4
        pin_out(8,6,3),                 // K3  LCD_VD5
        pin_out(8,5,3),                 // J1  LCD_VD6
        pin_out(7,1,4),                 // C14 LCD_VD7
        pin_out(7,5,3),                 // A7  LCD_VD8
        pin_out(4,8,2),                 // E2  LCD_VD9
        pin_out(4,10,2),                // M3  LCD_VD10
        pin_out(4,9,2),                 // L2  LCD_VD11
        pin_out(3,5,7),                 // C12 LCD_VD12
        pin_out(3,4,7),                 // A15 LCD_VD13
        pin_out(11,5,2),                // A12 LCD_VD14
        pin_out(11,4,2),                // B11 LCD_VD15
        pin_out(7,4,3),                 // C8  LCD_VD16
        pin_out(7,3,3),                 // C13 LCD_VD17
        pin_out(7,2,3),                 // A16 LCD_VD18
        pin_out(11,6,6),                // A6  LCD_VD19
        pin_out(11,3,2),                // A13 LCD_VD20
        pin_out(11,2,2),                // B12 LCD_VD21
        pin_out(11,1,2),                // A14 LCD_VD22
        pin_out(11,0,2),                // B15 LCD_VD23
    };

    for (int i = 0; i != sizeof pins / sizeof pins[0]; ++i) {
        unsigned pin = pins[i] >> 16;
        unsigned config = pins[i] & 0xffff;
        SFSP[0][pin] = config;
        //SFSP[pin >> 5][pin & 31] = config;
    }

    // Enable the lcd.
    *LCD_CTRL = 0x82d;                  // TFT, 16bpp, 565.
}


// Draw a 8 by 16 character, in colour.
void drawcharcolour (unsigned short * restrict target,
                     const unsigned * restrict bitmap,
                     unsigned foreground, unsigned background)
{
    unsigned flip = foreground ^ background;

    for (int i = 0; i != 4; ++i) {
        unsigned bits = *bitmap++;
        for (int j = 0; j != 4; ++j) {
            for (int k = 0; k != 8; ++k, bits >>= 1)
                *target++ = background ^ (flip & -(bits & 1));
            target += 1024 - 8;
        }
    }
}


// Draw a 8 by 16 character, white on black.
void drawcharbw (unsigned short * restrict target,
                 const unsigned * restrict bitmap)
{
    for (int i = 0; i != 4; ++i) {
        unsigned bits = *bitmap++;
        for (int j = 0; j != 4; ++j) {
            for (int k = 0; k != 8; ++k, bits >>= 1)
                *target++ = -(bits & 1);
            target += 1024 - 8;
        }
    }
}

static unsigned pc_x;
static unsigned pc_y;

typedef enum pc_state_t {
    s_normal,
    s_escape,
    s_command,
} pc_state_t;
static pc_state_t pc_state;


static void scroll(void)
{
    // Barrier because of aliasing.
    __memory_barrier();
    unsigned * target = (unsigned *) FRAMEBUFFER;
    unsigned * source = 512 * 16 + (unsigned *) FRAMEBUFFER;
    unsigned * end = 512 * 16 * TEXT_LINES + (unsigned *) FRAMEBUFFER;
    do {
        unsigned a = *source++;
        unsigned b = *source++;
        unsigned c = *source++;
        unsigned d = *source++;
        *target++ = a;
        *target++ = b;
        *target++ = c;
        *target++ = d;
    }
    while (source != end);
    do {
        *target++ = 0;
        *target++ = 0;
        *target++ = 0;
        *target++ = 0;
    }
    while (target != end);
    __memory_barrier();
    --pc_y;
}


static void clear_eol(void)
{
    __memory_barrier();
    unsigned * base = pc_x * 4 + pc_y * 16 * 512 + (unsigned *) FRAMEBUFFER;
    unsigned * end = 512 + pc_y * 16 * 512 + (unsigned *) FRAMEBUFFER;
    for (int i = 0; i != 16; ++i) {
        unsigned * line = base;
        do {
            *line++ = 0;
            *line++ = 0;
            *line++ = 0;
            *line++ = 0;
        }
        while (line != end);
        base += 512;
        end += 512;
    }
    __memory_barrier();
}


void lcd_putchar(unsigned char c)
{
    if (pc_state == s_normal && c >= 32) {
        if (pc_y == TEXT_LINES)
            scroll();
        drawcharbw(FRAMEBUFFER + pc_x * 8 + pc_y * 16 * 1024, characters[c]);
        ++pc_x;
        if (pc_x == 128) {
            pc_x = 0;
            ++pc_y;
        }
        return;
    }

    if (pc_state == s_normal) {            // Control.
        switch (c) {
        case '\r':
            pc_x = 0;
            return;
        case '\n':
            pc_x = 0;
            ++pc_y;
            if (pc_y == TEXT_LINES)
                scroll();
            return;
        case 27:
            pc_state = s_escape;
            return;
        default:
            return;
        }
    }

    switch (pc_state) {
    case s_escape:
        if (c == '[') {
            pc_state = s_command;
            return;
        }
    case s_command:
        if (c == 'K')
            clear_eol();
        break;
    default:
        break;
    }
    pc_state = s_normal;
}