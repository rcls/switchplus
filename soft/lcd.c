
#include "freq.h"
#include "lcd.h"
#include "pin.h"
#include "monkey.h"
#include "registers.h"
#include "sdram.h"

pixel_t FRAME_BUFFER[2097152] __aligned(8) __section("dram");

static volatile bool frame_flag;

void lcd_init (void)
{
    // Setup the PLL0AUDIO to give 74.75MHz off 50MHz ethernet clock.
    // ndec=122, mdec=13107, pdec=66
    // selr=0, seli=48, selp=24
    // pllfract = 47.839996,0x17eb85
    // pre-divider=16, feedback div=47, post-div=2
    // fcco=299MHz, fout=74.749994MHz.
    *PLL0AUDIO_CTRL = 0x03001811;
    *PLL0AUDIO_MDIV = 13107;
    *PLL0AUDIO_NP_DIV = 66 + (122 << 12);
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

    LCD->timh = (79 << 24) + (47 << 8) + (31 << 16) + 0xfc;
    LCD->timv = (16 << 24) + (2 << 16) + (9 << 10) + 1023;
    LCD->pol = 0x07ff3020;
    LCD->upbase = FRAME_BUFFER;         // SDRAM.
    LCD->lpbase = FRAME_BUFFER;
    LCD->ctrl = 0x1002c;                // TFT, 16bpp, disabled, watermark=8.

    // Setup pins.
    static const unsigned pins[] = {
        PIN_OUT_FAST(12,0,4),           // D4  LCD_DCLK
        PIN_OUT_FAST(4,6,2),            // C1  LCD_ENAB/LCDM
        PIN_OUT_FAST(4,5,2),            // D2  LCD_FP
        //PIN_OUT_FAST(,,), // B16 LCD_LE
        PIN_OUT_FAST(7,6,3),            // C7  LCD_LP
        //PIN_OUT_FAST(,,), // B6  LCD_PWR
        // PIN_OUT_FAST(4,1,2),            // A1  LCD_VD0
        // PIN_OUT_FAST(4,4,2),            // B1  LCD_VD1
        // PIN_OUT_FAST(4,3,2),            // C2  LCD_VD2
        PIN_OUT_FAST(4,2,2),            // D3  LCD_VD3
        PIN_OUT_FAST(8,7,3),            // K1  LCD_VD4
        PIN_OUT_FAST(8,6,3),            // K3  LCD_VD5
        PIN_OUT_FAST(8,5,3),            // J1  LCD_VD6
        PIN_OUT_FAST(7,1,4),            // C14 LCD_VD7
        // PIN_OUT_FAST(7,5,3),            // A7  LCD_VD8
        // PIN_OUT_FAST(4,8,2),            // E2  LCD_VD9
        PIN_OUT_FAST(4,10,2),           // M3  LCD_VD10
        PIN_OUT_FAST(4,9,2),            // L2  LCD_VD11
        PIN_OUT_FAST(3,5,7),            // C12 LCD_VD12
        PIN_OUT_FAST(3,4,7),            // A15 LCD_VD13
        PIN_OUT_FAST(11,5,2),           // A12 LCD_VD14
        PIN_OUT_FAST(11,4,2),           // B11 LCD_VD15
        // PIN_OUT_FAST(7,4,3),            // C8  LCD_VD16
        // PIN_OUT_FAST(7,3,3),            // C13 LCD_VD17
        // PIN_OUT_FAST(7,2,3),            // A16 LCD_VD18
        PIN_OUT_FAST(11,6,6),           // A6  LCD_VD19
        PIN_OUT_FAST(11,3,2),           // A13 LCD_VD20
        PIN_OUT_FAST(11,2,2),           // B12 LCD_VD21
        PIN_OUT_FAST(11,1,2),           // A14 LCD_VD22
        PIN_OUT_FAST(11,0,2),           // B15 LCD_VD23
    };

    config_pins(pins, sizeof pins / sizeof pins[0]);

    // Enable the lcd.
    LCD->ctrl = 0x1082d;                  // TFT, 16bpp, 565, watermark=8.

    // Enable the frame interrupt.
    LCD->intmsk = 4;
    NVIC_ISER[0] = 0x80;
}


void lcd_setframe_wait(const void * frame)
{
    LCD->upbase = frame;
    frame_flag = false;

    __interrupt_disable();
    while (!frame_flag)
        __interrupt_wait_go();
    __interrupt_enable();
}


void lcd_interrupt(void)
{
    unsigned interrupts = LCD->intstat;
    LCD->intclr = interrupts;
    if (interrupts & 4)
        frame_flag = true;
}
