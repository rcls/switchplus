#include "configure.h"
#include "monkey.h"
#include "usb.h"

#include <stddef.h>


// USB queue head.
typedef struct dQH_t {
    // 48 byte queue head.
    volatile unsigned capabilities;
    dTD_t * volatile current;

    dTD_t * volatile next;
    volatile unsigned length_and_status;
    void * volatile buffer_page[5];

    volatile unsigned reserved;
    volatile unsigned setup0;
    volatile unsigned setup1;
    // 16 bytes remaining for our use...
    dTD_t * first;
    dTD_t * last;                       // Only valid if first!=NULL.
    unsigned dummy2;
    unsigned dummy3;
} dQH_t;



#define NUM_DTDS 40
static struct qh_and_dtd_t {
    dQH_t QH[12];
    dTD_t DTD[NUM_DTDS];
} qh_and_dtd __aligned (2048) __section ("ahb0.qh_and_dtd");
_Static_assert(sizeof(qh_and_dtd) % 2048 == 0, "qh_and_dtd size");

static dTD_t * dtd_free_list;


static inline dQH_t * QH(int ep)
{
    return &qh_and_dtd.QH[ep];
}


const unsigned usb_init_regs[] __init_script("2") = {
    // Enable USB0 PHY power.
    BIT_RESET(*CREG0, 5),

    WORD_WRITE(USB->cmd, 2),            // Reset.
    BIT_WAIT_ZERO(USB->cmd, 1),

    WORD_WRITE(ENDPT->usbmode, 0xa),    // Device.  Tripwire.
    WORD_WRITE(ENDPT->otgsc, 9),
    //*PORTSC1 = 0x01000000;              // Only full speed for now.

    // Set the endpoint list pointer.
    WORD_WRITE32(USB->endpoint_list_addr, (unsigned) &qh_and_dtd.QH),
    WORD_WRITE(USB->device_addr, 0),

    WORD_WRITE(USB->cmd, 1),            // Run.

    // Pin H5, GPIO4[1] is a red LED for indicating fatal errors.
    // K4,  GPIO4[2] is a green LED which we leave on pull-up.  But set to
    // input as we may have just been loaded via DFU.
    WORD_WRITE(GPIO_WORD[4][1], 0),
    BIT_SET(GPIO_DIR[4], 1),
    BIT_RESET(GPIO_DIR[4], 2),

    WORD_WRITE(USB->intr, 0x00000041),  // Port change, reset, data.

};

void usb_init_mem(void)
{
    // Start with just the control end-points.
    for (int i = 0; i != sizeof qh_and_dtd.QH; ++i)
        ((char *) &qh_and_dtd.QH)[i] = 0;

    dtd_free_list = &qh_and_dtd.DTD[0];
    for (int i = 1; i < NUM_DTDS; ++i)
        qh_and_dtd.DTD[i-1].next = &qh_and_dtd.DTD[i];

    qh_init (EP_00, 0x20408000);
    qh_init (EP_80, 0x20408000);
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
        BIT_BAND(GPIO_DIR[4])[1] = 1;   // Turn on the red LED.
        GPIO_BYTE[4][1] = 0;
        puts ("Out of DTDs!!!\n");
    }
    // Do a softreset.
    *BASE_M4_CLK = 0x0e000800;          // Back to IDIVC.
    spin_for(100000);

    while (1)
        SCB->aircr = 0x05fa0004;
}


static void start_if_not_running(dQH_t * qh, dTD_t * d, unsigned ep)
{
    unsigned mask = ep_mask (ep);

    // 2. Read correct prime bit in ENDPTPRIME - if '1' DONE.
    if (ENDPT->prime & mask)
        return;

    unsigned eps;
    do {
        // 3. Set ATDTW bit in USBCMD register to '1'.
        BIT_BAND(USB->cmd)[14] = 1;

        // 4. Read correct status bit in ENDPTSTAT. (Store in temp variable
        // for later).
        eps = ENDPT->stat;

        // 5. Read ATDTW bit in USBCMD register.
        // - If '0' go to step 3.
        // - If '1' continue to step 6.
    }
    while (!BIT_BAND(USB->cmd)[14]);

    // 6. Write ATDTW bit in USBCMD register to '0'.
    // Seems unnecessary...
    //USB->cmd &= ~(1 << 14);

    // 7. If status bit read in step 4 (ENDPSTAT reg) indicates endpoint priming
    // is DONE (corresponding ERBRx or ETBRx is one): DONE.
    if (eps & mask)
        return;

    // We are used to restart-after-error when we are not sure if the endpoint
    // is running or not.  This means we need to distinguish between the case
    // where the DTD has completed and the case where it never started.
    if ((d->length_and_status & 0xff) != 0x80)
        return;

    // 8. If status bit read in step 4 is 0 then go to Linked list is empty:
    // Step 1.

    // 1. Write dQH next pointer AND dQH terminate bit to 0 as a single
    // DWord operation.
    qh->next = d;

    // 2. Clear active and halt bits in dQH (in case set from a previous
    // error).
    qh->length_and_status &= ~0xc0;

    // 3. Prime endpoint by writing '1' to correct bit position in
    // ENDPTPRIME.
    ENDPT->prime = mask;
}


void schedule_dtd (unsigned ep, dTD_t * dtd)
{
    dQH_t * qh = QH (ep);

    dtd->next = (dTD_t *) 1;
    if (qh->first != NULL)
        qh->last->next = dtd;           // 1. Add dTD to end of the linked list.
    else
        qh->first = dtd;

    qh->last = dtd;

    start_if_not_running(qh, dtd, ep);
}


void schedule_buffer (unsigned ep, void * data, unsigned length,
                      dtd_completion_t * cb)
{
    dTD_t * dtd = get_dtd();

    // Set interrupt & active bits.
    dtd->length_and_status = (length << 16) + 0x8080;
    dtd->buffer_page[0] = data;
    dtd->buffer_page[1] = (void *) (0xfffff000 & (unsigned) data) + 4096;
    // We don't do anything this big; just save original address.
    dtd->buffer_page[4] = data;
    dtd->completion = cb;

    schedule_dtd (ep, dtd);
}


static void retire_dtd (dTD_t * d, dQH_t * qh)
{
    qh->first = (dTD_t *) ((unsigned) d->next & ~1);

    if (d->completion) {
        unsigned status = d->length_and_status;
        d->completion(d, status & 0xff, status >> 16);
    }

    d->next = dtd_free_list;
    dtd_free_list = d;
}


void endpt_complete(unsigned ep)
{
    dQH_t * qh = QH (ep);

    while (qh->first) {
        dTD_t * d = qh->first;
        unsigned status = d->length_and_status & 0xff;
        if (status == 0x80)
            return;                     // Still running.

        // If something went wrong, restart the end-point if necessary.
        if (status && (unsigned) d->next != 1)
            start_if_not_running(qh, d->next, ep);

        retire_dtd(d, qh);              // May frig the qh...
    }
}


void endpt_clear(unsigned ep)
{
    // Endpt is stopped; clear it out.
    dQH_t * qh = QH (ep);

    while (qh->first)
        retire_dtd(qh->first, qh);
}


void endpt_complete_one(unsigned ep)
{
    dQH_t * qh = QH (ep);
    if (!qh->first)
        return;

    if ((qh->first->length_and_status & 0xff) == 0x80) {
        ENDPT->flush = ep_mask(ep);
        while (ENDPT->flush & ep_mask(ep));
        qh->first->length_and_status |= 1; // Will force restart.
    }

    endpt_complete(ep);
}


void qh_init (unsigned ep, unsigned capabilities)
{
    dQH_t * qh = QH (ep);
    // If no transfer size given, then assume it's a bulk end-point.
    if ((capabilities & 0x07ff0000) == 0)
        capabilities += is_high_speed() ? 0x02000000 : 0x00400000;

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

    schedule_buffer (EP_80, (void*) buffer, length,
                     length == 0 ? callback : NULL);

    if (ENDPT->setupstat & 1)
        puts ("Oops, EPSS\n");

    if (length == 0)
        return;                         // No data so no ack...

    // Now the status dtd...
    schedule_buffer (EP_00, NULL, 0, callback);

    if (ENDPT->setupstat & 1)
        puts ("Oops, EPSS\n");
}


unsigned get_0_setup (unsigned * setup1)
{
    unsigned setup0;
    do {
        BIT_BAND(USB->cmd)[13] = 1;     // Set tripwire.
        setup0 = QH(EP_00)->setup0;
        *setup1 = QH(EP_00)->setup1;
    }
    while (!BIT_BAND(USB->cmd)[13]);
    //BIT_BAND(USB->cmd)[13] = 0;
    while (ENDPT->setupstat & 1)
        ENDPT->setupstat = 1;

    return setup0;
}
