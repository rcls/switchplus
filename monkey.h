// The monkey logger.  Sends stuff to serial and/or a USB serial.
#ifndef MONKEY_H_
#define MONKEY_H_

#include <stdbool.h>

#define log_monkey true

extern bool debug_flag;
extern bool verbose_flag;

void init_monkey_usb (void);

void monkey_kick (void);
bool monkey_is_empty (void);

void putchar (int byte);
void puts (const char * s);
void printf (const char * __restrict__ format, ...)
    __attribute__ ((format (printf, 1, 2)));

int getchar (void);
void ungetchar (int c);
int peekchar_nb (void);

#define debugf !debug_flag ? (void)0 : printf
#define verbose !verbose_flag ? (void)0 : printf

#endif
