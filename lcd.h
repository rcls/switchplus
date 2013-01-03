#ifndef LCD_H
#define LCD_H

void lcd_init (void);

void drawcharcolour (unsigned short * restrict target,
                     const unsigned * restrict bitmap,
                     unsigned foreground, unsigned background);
void drawcharbw (unsigned short * restrict target,
                 const unsigned * restrict bitmap);

void lcd_putchar (unsigned char c);

#endif
