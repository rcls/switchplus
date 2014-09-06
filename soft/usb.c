#include "usb.h"
#include "monkey.h"

#include <stddef.h>


// USB queue head.
typedef struct dQH_t {
    // 48 byte queue head.
    volatile unsigned capabilities;
    dTD_t * volatile current;

    dTD_t * volatile next;
    volatile unsigned length_and_status;
    volatile unsigned buffer_page[5];

    volatile unsigned reserved;
    volatile unsigned setup0;
    volatile unsigned setup1;
    // 16 bytes remaining for our use...
    dTD_t * first;
    dTD_t * last;
    unsigned dummy2;
    unsigned dummy3;
} dQH_t;


typedef struct qh_pair_t {
    dQH_t OUT;                          // OUT is host to device.
    dQH_t IN;                           // IN is device to host.
} qh_pair_t;

#define NUM_DTDS 40
static struct qh_and_dtd_t {
    qh_pair_t QH[6];
    dTD_t DTD[NUM_DTDS];
} qh_and_dtd __aligned (2048) __section ("ahb0.qh_and_dtd");

static dTD_t * dtd_free_list;


static inline dQH_t * QH(int ep)
{
    return ep & 0x80 ? &qh_and_dtd.QH[ep - 0x80].IN : &qh_and_dtd.QH[ep].OUT;
}


static void put_dtd (dTD_t * dtd)
{
    dtd->next = dtd_free_list;
    dtd_free_list = dtd;
}


void usb_init (void)
{
    // Enable USB0 PHY power.
    *CREG0 &= ~32;

    *USBCMD = 2;                        // Reset.
    while (*USBCMD & 2);

    dtd_free_list = NULL;
    for (int i = 0; i != NUM_DTDS; ++i)
        put_dtd (&qh_and_dtd.DTD[i]);

    *USBMODE = 0xa;                     // Device.  Tripwire.
    *OTGSC = 9;
    //*PORTSC1 = 0x01000000;              // Only full speed for now.

    // Start with just the control end-points.
    for (int i = 0; i != sizeof qh_and_dtd.QH; ++i)
        ((char *) &qh_and_dtd.QH)[i] = 0;

    qh_init (0x00, 0x20408000);
    qh_init (0x80, 0x20408000);

    // Set the endpoint list pointer.
    *ENDPOINTLISTADDR = (unsigned) &qh_and_dtd.QH;
    *DEVICEADDR = 0;

    *USBCMD = 1;                        // Run.

    // Pin H5, GPIO4[1] is a LED for indicating fatal errors.
    GPIO_DIR[4] |= 2;
    GPIO_BYTE[4][1] = 0;
}


dTD_t * get_dtd (void)
{
    dTD_t * r = dtd_free_list;
    if (r != NULL) {
        dtd_free_list = r->next;
        return r;
    }

    static volatile bool reenter;
    if (!reenter) {
        reenter = true;
        GPIO_DIR[4] |= 1 << 1;
        GPIO_BYTE[4][1] = 0;
        puts ("Out of DTDs!!!\n");
    }
    // Do a softreset.
    *BASE_M4_CLK = 0x0e000800;          // Back to IDIVC.
    for (int i = 0; i != 100000; ++i)
        asm volatile ("");
    while (1)
        *CORTEX_M_AIRCR = 0x05fa0004;
}


void schedule_dtd (unsigned ep, dTD_t * dtd)
{
    dQH_t * qh = QH (ep);
    ep = ep_mask (ep);

    dtd->next = (dTD_t *) 1;
    if (qh->last != NULL) {
        // 1. Add dTD to end of the linked list.
        qh->last->next = dtd;
        qh->last = dtd;

        // 2. Read correct prime bit in ENDPTPRIME - if '1' DONE.
        if (*ENDPTPRIME & ep)
            return;

        unsigned eps;
        do {
            // 3. Set ATDTW bit in USBCMD register to '1'.
            *USBCMD |= 1 << 14;

            // 4. Read correct status bit in ENDPTSTAT. (Store in temp variable
            // for later).
            eps = *ENDPTSTAT;

            // 5. Read ATDTW bit in USBCMD register.
            // - If '0' go to step 3.
            // - If '1' continue to step 6.
        }
        while (!(*USBCMD & (1 << 14)));

        // 6. Write ATDTW bit in USBCMD register to '0'.
        // Seems unnecessary...
        //*USBCMD &= ~(1 << 14);

        // 7. If status bit read in step 4 (ENDPSTAT reg) indicates endpoint
        // priming is DONE (corresponding ERBRx or ETBRx is one): DONE.
        if (eps & ep)
            return;

        // 8. If status bit read in step 4 is 0 then go to Linked list is empty:
        // Step 1.
    }
    else {
        qh->first = dtd;
        qh->last = dtd;
    }

    // 1. Write dQH next pointer AND dQH terminate bit to 0 as a single
    // DWord operation.
    qh->next = dtd;

    // 2. Clear active and halt bits in dQH (in case set from a previous
    // error).
    qh->length_and_status &= ~0xc0;

    // 3. Prime endpoint by writing '1' to correct bit position in
    // ENDPTPRIME.
    *ENDPTPRIME = ep;
}


void schedule_buffer (unsigned ep, void * data, unsigned length,
                      dtd_completion_t * cb)
{
    dTD_t * dtd = get_dtd();

    // Set interrupt & active bits.
    dtd->length_and_status = (length << 16) + 0x8080;
    dtd->buffer_page[0] = (unsigned) data;
    dtd->buffer_page[1] = (0xfffff000 & (unsigned) data) + 4096;
    dtd->buffer_page[2] = (0xfffff000 & (unsigned) data) + 8192;
    dtd->buffer_page[3] = (0xfffff000 & (unsigned) data) + 12288;
    // We don't do anything this big; just save original address.
    dtd->buffer_page[4] = (unsigned) data;
    dtd->completion = cb;

    schedule_dtd (ep, dtd);
}

static dTD_t * retire_dtd (dTD_t * d, dQH_t * qh)
{
    dTD_t * next = d->next;
    if (d == qh->last) {
        next = NULL;
        qh->last = NULL;
    }
    qh->first = next;

    put_dtd (d);
    return next;
}


void endpt_complete (unsigned ep, bool running)
{
    dQH_t * qh = QH (ep);

    // Successes...
    for (dTD_t * d = qh->first; d; ) {
        unsigned status = d->length_and_status;
        if ((status & 0xff) == 0x80 && running)
            break;                      // Still running.

        if (status & 0x7f)
            // Errored.
            printf ("ERROR ep %02x length and status: %08x\n", ep, status);

        if (d->completion)
            d->completion (d, status & 0xff, status >> 16);
        d = retire_dtd (d, qh);

        if (d && (status & 0x80) && running) {
            qh->next = d;               // Restart the end-point.
            qh->length_and_status &= ~0xc0;
            *ENDPTPRIME = ep_mask(ep);
            break;
        }
    }
}


void qh_init (unsigned ep, unsigned capabilities)
{
    dQH_t * qh = QH (ep);
    qh->capabilities = capabilities;
    qh->next = (void *) 1;
}


void respond_to_setup (unsigned setup1, const void * buffer, unsigned length,
                       dtd_completion_t * callback)
{
    if ((setup1 >> 16) < length)
        length = setup1 >> 16;

    // The DMA won't take this into account...
    /* if (descriptor && (unsigned) descriptor < 0x10000000) */
    /*     descriptor += * M4MEMMAP; */

    schedule_buffer (0x80, (void*) buffer, length,
                     length == 0 ? callback : NULL);

    if (*ENDPTSETUPSTAT & 1)
        puts ("Oops, EPSS\n");

    if (length == 0)
        return;                         // No data so no ack...

    // Now the status dtd...
    schedule_buffer (0, NULL, 0, callback);

    if (*ENDPTSETUPSTAT & 1)
        puts ("Oops, EPSS\n");
}


unsigned get_0_setup (unsigned * setup1)
{
    unsigned setup0;
    do {
        *USBCMD |= 1 << 13;             // Set tripwire.
        setup0 = QH(0)->setup0;
        *setup1 = QH(0)->setup1;
    }
    while (!(*USBCMD & (1 << 13)));
    *USBCMD &= ~(1 << 13);
    while (*ENDPTSETUPSTAT & 1)
        *ENDPTSETUPSTAT = 1;

    return setup0;
}
