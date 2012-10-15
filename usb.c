#include "usb.h"
#include "monkey.h"

#include <stddef.h>

// OUT is host to device.
// IN is device to host.
typedef struct qh_pair_t {
    dQH_t OUT;
    dQH_t IN;
} qh_pair_t;

#define NUM_DTDS 40
static struct qh_and_dtd_t {
    qh_pair_t QH[6];
    dTD_t DTD[NUM_DTDS];
} qh_and_dtd __attribute__ ((aligned (2048)));
#define QH qh_and_dtd.QH
#define DTD qh_and_dtd.DTD


static dTD_t * dtd_free_list;


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
        put_dtd (&DTD[i]);

    *USBMODE = 0xa;                     // Device.  Tripwire.
    *OTGSC = 9;
    //*PORTSC1 = 0x01000000;              // Only full speed for now.

    qh_init (0x00, 0x20408000);
    qh_init (0x80, 0x20408000);

    // Set the endpoint list pointer.
    *ENDPOINTLISTADDR = (unsigned) &QH;
    *DEVICEADDR = 0;

    *USBCMD = 1;                        // Run.

    // Pin H5, GPIO4[1] is a LED for indicating fatal errors.
    GPIO_DIR[4] |= 2;
    GPIO_BYTE[4][1] = 0;
}


dTD_t * get_dtd (void)
{
    dTD_t * r = dtd_free_list;
    if (r == NULL) {
        log_serial = true;
        puts ("Out of DTDs!!!\r\n");
        /* ser_w_hex (tx_dma_retire, 8, " "); */
        /* ser_w_hex (tx_dma_insert, 8, " tx retire, insert\r\n"); */
        /* ser_w_hex (rx_dma_retire, 8, " "); */
        /* ser_w_hex (rx_dma_insert, 8, " rx retire, insert\r\n"); */
        GPIO_BYTE[4][1] = 0;
        while (1)
            asm volatile ("wfi\n");
    }
    dtd_free_list = r->next;
    return r;
}


void schedule_dtd (unsigned ep, dTD_t * dtd)
{
    dQH_t * qh;
    if (ep >= 0x80) {                   // IN.
        qh = &QH[ep - 0x80].IN;
        ep = 0x10000 << (ep - 0x80);
    }
    else {                              // OUT.
        qh = &QH[ep].OUT;
        ep = 1 << ep;
    }

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
    while (*ENDPTPRIME & ep);
    if (!(*ENDPTSTAT & ep))
        puts ("Oops, EPST\r\n");
}


bool schedule_buffer (unsigned ep, const void * data, unsigned length,
                      dtd_completion_t * cb)
{
    dTD_t * dtd = get_dtd();
    if (dtd == NULL)
        return false;                   // Bugger.

    // Set terminate & active bits.
    dtd->length_and_status = (length << 16) + 0x8080;
    dtd->buffer_page[0] = (unsigned) data;
    dtd->buffer_page[1] = (0xfffff000 & (unsigned) data) + 4096;
    dtd->buffer_page[2] = (0xfffff000 & (unsigned) data) + 8192;
    dtd->buffer_page[3] = (0xfffff000 & (unsigned) data) + 12288;
    // We don't do anything this big; just save original address.
    dtd->buffer_page[4] = (unsigned) data;
    dtd->completion = cb;

    schedule_dtd (ep, dtd);
    return true;
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


void endpt_complete (unsigned ep, bool reprime)
{
    dQH_t * qh;
    unsigned mask;

    if (ep & 0x80) {
        qh = &QH[ep - 0x80].IN;
        mask = 0x10000 << (ep - 0x80);
    }
    else {
        qh = &QH[ep].OUT;
        mask = 1 << ep;
    }

    // Clean-up the DTDs...
    if (qh->first == NULL)
        return;

    // Successes...
    dTD_t * d = qh->first;
    while (!(d->length_and_status & 0x80)) {
        //ser_w_hex (d->length_and_status, 8, " ok length and status\r\n");
        if (d->completion)
            d->completion (ep, qh, d);
        d = retire_dtd (d, qh);
        if (d == NULL)
            return;
    }

    if (!(d->length_and_status & 0x7f))
        return;                         // Still going.

    // FIXME - what do we actually want to do on errors?
    printf ("ERROR length and status: %08x\r\n", d->length_and_status);
    if (d->completion)
        d->completion (ep, qh, d);
    if (retire_dtd (d, qh) && reprime)
        *ENDPTPRIME = mask;             // Reprime the endpoint.
}


void qh_init (unsigned ep, unsigned capabilities)
{
    dQH_t * qh;

    if (ep & 0x80)
        qh = &QH[ep - 0x80].IN;
    else
        qh = &QH[ep].OUT;

    qh->capabilities = capabilities;
    qh->next = (void *) 1;
}


unsigned long long get_0_setup (void)
{
    unsigned setup0;
    unsigned setup1;
    do {
        *USBCMD |= 1 << 13;             // Set tripwire.
        setup0 = QH[0].OUT.setup0;
        setup1 = QH[0].OUT.setup1;
    }
    while (!(*USBCMD & (1 << 13)));
    *USBCMD &= ~(1 << 13);
    while (*ENDPTSETUPSTAT & 1)
        *ENDPTSETUPSTAT = 1;

    return setup1 * 0x100000000ull + setup0;
}
