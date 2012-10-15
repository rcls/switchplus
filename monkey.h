// The monkey logger.  Sends stuff to serial and/or a USB serial.
#ifndef MONKEY_H_
#define MONKEY_H_

#include <stdbool.h>

extern bool log_serial;
#define log_monkey true

void init_monkey (void);

void monkey_kick (void);

void ser_w_byte (unsigned byte);
void ser_w_string (const char * s);
void ser_w_hex (unsigned value, int nibbles, const char * term);

#endif
