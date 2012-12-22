
#include "lcd.h"
#include "monkey.h"
#include "registers.h"

void lcd_init (void)
{
    // Setup the PLL0AUDIO to give 52.25MHz off 52MHz ethernet clock.
    // ndec=5, mdec=32426, pdec=5
    // selr=0, seli=28, selp=14
    // pllfract = 26.125,0x0d1000
    // pre-divider=5, feedback div=26, post-div=5
    // fcco=522.5MHz, fout=52.25MHz.
    *PLL0AUDIO_CTRL = 0x03001811;
    *PLL0AUDIO_MDIV = 32426;
    *PLL0AUDIO_NP_DIV = (5 << 12) + 5;
    *PLL0AUDIO_FRAC = 0x0d1000;

    *PLL0AUDIO_CTRL = 0x03001810;
    puts ("LCD lock wait");
    while (!(*PLL0AUDIO_STAT & 1));
    puts ("\n");

    // The lcd clock is outclk11.  PLL0AUDIO is clock source 8.
    *BASE_LCD_CLK = 0x08000800;

    // Enable the lcd register clock branch.
    *CLK_M4_LCD_CFG = 1;

    // Reset the lcd.
    RESET_CTRL[0] = 1 << 16;

    for (int i = 0; i != 10000; ++i)
        asm volatile("");

    // 1024x640 59.89 Hz (CVT 0.66MA) hsync: 39.82 kHz; pclk: 52.25 MHz
    //Modeline "1024x640_60.00"   52.25  1024 1072 1168 1312  640 643 649 665
    //-hsync +vsync
    // horizontal 1024 48 96 144
    // vertical 640 3 6 16

    *LCD_TIMH = 0x8f2f5ffc;
    *LCD_TIMV = 0x0f02167f;
    *LCD_POL = 0x07ff3020;
    *LCD_UPBASE = 0x60000000;           // SDRAM.
    *LCD_LPBASE = 0x60000000;
    *LCD_CTRL = 0x28;                   // TFT, 16bpp, disabled.

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
        pin_out(8,4,3),                 // J2  LCD_VD7
        pin_out(7,5,3),                 // A7  LCD_VD8
        pin_out(4,8,2),                 // E2  LCD_VD9
        pin_out(4,10,2),                // M3  LCD_VD10
        pin_out(4,9,2),                 // L2  LCD_VD11
        pin_out(8,3,3),                 // J3  LCD_VD12
        pin_out(4,0,5),                 // D5  LCD_VD13
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
    *LCD_CTRL = 0x829;                  // TFT, 16bpp.
}
