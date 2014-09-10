// USB buffer / endpoint handling.
#ifndef USBDRIVER_H_
#define USBDRIVER_H_

#include "registers.h"

#include <stdbool.h>

typedef struct dTD_t dTD_t;

typedef void dtd_completion_t (dTD_t * dtd, unsigned status, unsigned remain);

// USB transfer descriptor.
struct dTD_t {
    struct dTD_t * volatile next;
    volatile unsigned length_and_status;
    volatile unsigned buffer_page[5];

    dtd_completion_t * completion;      // For our use...
};

void usb_init (void);
void qh_init (unsigned ep, unsigned capabilities);

void respond_to_setup (unsigned setup1, const void * buffer, unsigned length,
                       dtd_completion_t * callback);

dTD_t * get_dtd (void);
void schedule_dtd (unsigned ep, dTD_t * dtd);
void schedule_buffer (unsigned ep, void * data, unsigned length,
                      dtd_completion_t * cb);

bool endpt_complete(unsigned ep);       // Deal with finished dtds.
void endpt_clear(unsigned ep);          // Clear out stopped end-point.

unsigned get_0_setup (unsigned * setup1);

static inline unsigned ep_mask (unsigned ep)
{
    return ep & 0x80 ? 0x10000 << (ep - 0x80) : 1 << ep;
}


#endif
