// USB buffer / endpoint handling.
#ifndef USBDRIVER_H_
#define USBDRIVER_H_

#include "registers.h"

#include <stdbool.h>

void usb_init (void);
void qh_init (unsigned ep, unsigned capabilities);

void respond_to_setup (unsigned setup1, const void * buffer, unsigned length,
                       dtd_completion_t * callback);

dTD_t * get_dtd (void);
void schedule_dtd (unsigned ep, dTD_t * dtd);
void schedule_buffer (unsigned ep, const void * data, unsigned length,
                      dtd_completion_t * cb);
void endpt_complete (unsigned ep, bool running);

unsigned long long get_0_setup (void);

#endif
