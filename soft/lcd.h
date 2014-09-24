#ifndef LCD_H
#define LCD_H

void lcd_init (void);

void lcd_setframe_wait (const void * frame);

void lcd_interrupt (void);

typedef unsigned short pixel_t;
extern pixel_t FRAME_BUFFER[2097152];

#endif
