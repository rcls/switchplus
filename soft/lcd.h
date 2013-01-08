#ifndef LCD_H
#define LCD_H

void lcd_init (void);

void lcd_setframe_wait (const void * frame);

void lcd_interrupt (void);

#endif
