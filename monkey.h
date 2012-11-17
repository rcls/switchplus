// The monkey logger.  Sends stuff to serial and/or a USB serial.
#ifndef MONKEY_H_
#define MONKEY_H_

#include <stdbool.h>

extern bool log_serial;
#define log_monkey true

void init_monkey_serial (void);

void monkey_kick (void);

void putchar (int byte);
void puts (const char * s);
void printf (const char * __restrict__ format, ...)
    __attribute__ ((format (printf, 1, 2)));

#endif
