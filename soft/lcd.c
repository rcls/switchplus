
#include "configure.h"
#include "freq.h"
#include "lcd.h"
#include "monkey.h"
#include "registers.h"
#include "sdram.h"

pixel_t FRAME_BUFFER[2097152] __aligned(8) __section("dram");

static volatile bool frame_flag;

void lcd_init (void)
{
    // Reset the SDRAM.
    meminit (cpu_frequency (1));

    puts ("LCD setup:");
    static const unsigned pins[] = {
        // Setup the PLL0AUDIO to give 74.75MHz off 50MHz ethernet clock.
        // ndec=122, mdec=13107, pdec=66
        // selr=0, seli=48, selp=24
        // pllfract = 47.839996,0x17eb85
        // pre-divider=16, feedback div=47, post-div=2
        // fcco=299MHz, fout=74.749994MHz.
        WORD_WRITE32n(PLL0AUDIO->ctrl, 4,
                      0x03001811,       // CTRL
                      13107,            // MDIV
                      66 + (122 << 12), // NP_DIV
                      0x17eb85),        // FRAC

        BIT_RESET(PLL0AUDIO->ctrl, 0),

        // Wait for lock.
        BIT_WAIT_ZERO(PLL0AUDIO->stat, 0),

        // The lcd clock is outclk11.  PLL0AUDIO is clock source 8.
        WORD_WRITE32(*BASE_LCD_CLK, 0x08000800),

        // Reset the lcd.
        WORD_WRITE32(RESET_CTRL[0], 1<<16),
        BIT_WAIT_SET(RESET_ACTIVE_STATUS[0], 16),

        // 1024x1024 59.90 Hz (CVT) hsync: 63.13 kHz; pclk: 74.75 MHz
        // Modeline "1024x1024R" 74.75  1024 1072 1104 1184  1024 1027 1037 1054
        // +hsync -vsync
        // horizontal 1024 48 32 80
        // vertical 1024 3 10 17

        WORD_WRITE32n(LCD->timh, 7,
                      (79 << 24) + (47 << 8) + (31 << 16) + 0xfc, // TIMH
                      (16 << 24) + (2 << 16) + (9 << 10) + 1023,  // TIMV
                      0x07ff3020,                                 // POL
                      0,                                          // LE
                      (unsigned) FRAME_BUFFER,                    // UPBASE
                      (unsigned) FRAME_BUFFER,                    // LPBASE
                      0x1082c),      // CTRL: TFT, 16bpp, disabled, watermark=8.

        // PIN_OUT_FAST(4,1,2),            // A1  LCD_VD0
        // PIN_OUT_FAST(4,3,2),            // C2  LCD_VD2
        // PIN_OUT_FAST(4,4,2),            // B1  LCD_VD1
        // PIN_OUT_FAST(4,8,2),            // E2  LCD_VD9
        // PIN_OUT_FAST(7,2,3),            // A16 LCD_VD18
        // PIN_OUT_FAST(7,3,3),            // C13 LCD_VD17
        // PIN_OUT_FAST(7,4,3),            // C8  LCD_VD16
        // PIN_OUT_FAST(7,5,3),            // A7  LCD_VD8
        //PIN_OUT_FAST(,,), // B16 LCD_LE
        //PIN_OUT_FAST(,,), // B6  LCD_PWR
        PIN_OUT_FAST(3,4,7)             // A15 LCD_VD13
        + PIN_EXTRA(1),                 // C12 LCD_VD12
        PIN_OUT_FAST(4,2,2),            // D3  LCD_VD3
        PIN_OUT_FAST(4,5,2)             // D2  LCD_FP
        + PIN_EXTRA(1),                 // C1  LCD_ENAB/LCDM
        PIN_OUT_FAST(4,9,2)             // L2  LCD_VD11
        + PIN_EXTRA(1),                 // M3  LCD_VD10
        PIN_OUT_FAST(7,1,4),            // C14 LCD_VD7
        PIN_OUT_FAST(7,6,3),            // C7  LCD_LP
        PIN_OUT_FAST(8,5,3)             // J1  LCD_VD6
        + PIN_EXTRA(2),                 // K3  LCD_VD5, K1  LCD_VD4
        PIN_OUT_FAST(11,0,2)            // B15 LCD_VD23
        + PIN_EXTRA(6),         // ... A13 VD20, B11 VD15, A12 VD14, A6 VD19
        PIN_OUT_FAST(12,0,4),           // D4  LCD_DCLK

        // Enable the lcd.
        BIT_SET(LCD->ctrl, 0), // TFT, 16bpp, 565, watermark=8.

        // Enable the frame interrupt.
        WORD_WRITE(LCD->intmsk, 4),
    };

    configure(pins, sizeof pins / sizeof pins[0]);

    NVIC_ISER[0] = 1 << m4_lcd;

    puts (" done\n");
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
