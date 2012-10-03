// Lets try bring ub a usb device...

#include "registers.h"

#include <stdbool.h>
#include <stddef.h>

#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)
#define STATIC_ASSERT(b) int JOIN(_sa_dummy_, __LINE__)[b ? 1 : -1]

typedef struct dTD_t {
    struct dTD_t * volatile next;
    volatile unsigned length_and_status;
    unsigned volatile buffer_page[5];

    unsigned dummy;                     // Hopefully we can use this.
} __attribute__ ((aligned (32))) dTD_t;

typedef struct dQH_t {
    // 48 byte queue head.
    volatile unsigned capabilities;
    dTD_t * volatile current;

    dTD_t * volatile next;
    volatile unsigned length_and_status;
    unsigned volatile buffer_page[5];

    volatile unsigned reserved;
    volatile unsigned setup0;
    volatile unsigned setup1;
    // 16 bytes remaining for our use...
    dTD_t * first;
    dTD_t * last;
    unsigned dummy2;
    unsigned dummy3;
} __attribute__ ((aligned (64))) dQH_t;

// OUT is host to device.
// IN is device to host.
typedef struct qh_pair_t {
    dQH_t OUT;
    dQH_t IN;
} qh_pair_t;

static qh_pair_t QH[6] __attribute__ ((aligned (2048)));

#define NUM_DTDS 52
static dTD_t DTD[NUM_DTDS] __attribute__ ((aligned (32)));

static dTD_t * dtd_free_list;

#define DEVICE_DESCRIPTOR_SIZE 18
static const unsigned char device_descriptor[] = {
    DEVICE_DESCRIPTOR_SIZE,
    1,                                  // type:
    0, 2,                               // bcdUSB.
    2,                                  // class - vendor specific.
    6,                                  // subclass.
    0,                                  // protocol.
    64,                                 // Max packet size.
    0x55, 0xf0,                         // Vendor-ID.
    'L', 'R',                           // Device-ID.
    0x34, 0x12,                         // Revision number.
    1,                                  // Manufacturer string index.
    2,                                  // Product string index.
    3,                                  // Serial number string index.
    1                                   // Number of configurations.
    };
STATIC_ASSERT (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor));


#define CONFIG_DESCRIPTOR_SIZE (9 + 9 + 5 + 13 + 5 + 7 + 9 + 9 + 7 + 7)
static const unsigned char config_descriptor[] = {
    // Config.
    9,                                  // length.
    2,                                  // type: config.
    CONFIG_DESCRIPTOR_SIZE & 0xff,      // size.
    CONFIG_DESCRIPTOR_SIZE >> 8,
    2,                                  // num interfaces.
    1,                                  // configuration number.
    0,                                  // string descriptor index.
    0x80,                               // attributes, not self powered.
    250,                                // current (500mA).
    // Interface (comm).
    9,                                  // length.
    4,                                  // type: interface.
    0,                                  // interface number.
    0,                                  // alternate setting.
    1,                                  // number of endpoints.
    2,                                  // interface class.
    6,                                  // interface sub-class.
    0,                                  // protocol.
    0,                                  // interface string index.
    // CDC header...
    5,
    0x24,
    0,
    0x10, 1,
    // Ethernet functional descriptor.
    13,
    0x24,                               // cs_interface,
    15,                                 // ethernet
    4,                                  // Mac address string.
    0, 0, 0, 0,                         // Statistics bitmap.
    0, 7,                               // Max segment size.
    0, 0,                               // Multicast filters.
    0,                                  // Number of power filters.
    // Union...
    5,
    0x24,
    6,
    0,                                  // Interface 0 is control.
    1,                                  // Interface 1 is sub-ordinate.
    // Endpoint.
    7,
    5,
    0x81,                               // IN 1
    3,                                  // Interrupt
    64, 0,                              // 64 bytes
    11,                                 // binterval
    // Endpoint.
    // 7,
    // 5,
    // 0x81,                               // IN 1
    // 3,                                  // Interrupt
    // 64, 0,                              // 64 bytes
    // 11,                                 // binterval
    // Interface (data).
    9,                                  // length.
    4,                                  // type: interface.
    1,                                  // interface number.
    0,                                  // alternate setting.
    0,                                  // number of endpoints.
    10,                                 // interface class (data).
    6,                                  // interface sub-class.
    0,                                  // protocol.
    0,                                  // interface string index.
    // Interface (data).
    9,                                  // length.
    4,                                  // type: interface.
    1,                                  // interface number.
    1,                                  // alternate setting.
    2,                                  // number of endpoints.
    10,                                 // interface class (data).
    6,                                  // interface sub-class.
    0,                                  // protocol.
    0,                                  // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    2,                                  // OUT 2.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x82,                               // IN 2.
    0x2,                                // bulk
    0, 2,                               // packet size
    0
};
STATIC_ASSERT (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor));

// String0 - lang. descs.
static const unsigned char string0[] = {
    4, 3, 9, 4
};
static const unsigned char string1[] = {
    12, 3, 'R', 0, 'a', 0, 'l', 0, 'p', 0, 'h', 0
};
static const unsigned char string2[] = {
    14, 3, 'S', 0, 'w', 0, 'i', 0, 't', 0, 'c', 0, 'h', 0
};
static const unsigned char string3[] = {
    10, 3, '0', 0, '0', 0, '0', 0, '1', 0
};
static const unsigned char string4[] = {
    26, 3,
    '4', 0, '2', 0, '4', 0, '2', 0, '4', 0, '2', 0,
    '4', 0, '2', 0, '4', 0, '2', 0, '4', 0, '2', 0
};
static const unsigned char * const string_descriptors[] = {
    string0, string1, string2, string3, string4
};


static const unsigned char network_connected[] = {
    0xa1, 0, 1, 0, 0, 0, 0, 0
};
// static const unsigned char network_disconnected[] = {
//     0xa1, 0, 0, 0, 0, 0, 0, 0
// };

static const unsigned char speed_notification100[] = {
    0xa1, 0x2a, 0, 0, 0, 0, 0, 8,
    0, 00, 0xe1, 0x5e, 0x5f, 00, 0xe1, 0x5e, 0x5f,
};


#if 0
#define QUALIFIER_DESCRIPTOR_SIZE 10
const unsigned char qualifier_descriptor[] = {
    QUALIFIER_DESCRIPTOR_SIZE,          // Length.
    6,                                  // Type
    0, 2,                               // usb version
    255, 1, 1,
    64, 1, 0
};
STATIC_ASSERT (QUALIFIER_DESCRIPTOR_SIZE == sizeof (qualifier_descriptor));
#endif

//static unsigned char rx_ring_buffer[8192] __attribute__ ((aligned (4096)));
static unsigned char tx_ring_buffer[8192] __attribute__ ((aligned (4096)));

static void ser_w_byte (unsigned byte)
{
#if 1
    while (!(*USART3_LSR & 32));         // Wait for THR to be empty.
    *USART3_THR = byte;
#endif
}

static void ser_w_string (const char * s)
{
    for (; *s; s++)
        ser_w_byte (*s);
}

static void ser_w_hex (unsigned value, int nibbles, const char * term)
{
    for (int i = nibbles; i != 0; ) {
        --i;
        ser_w_byte("0123456789abcdef"[(value >> (i * 4)) & 15]);
    }
    ser_w_string (term);
}


static dTD_t * get_dtd (void)
{
    dTD_t * r = dtd_free_list;
    if (r != NULL)
        dtd_free_list = r->next;
    else
        ser_w_string ("Out of DTDs!!!\r\n");
    return r;
}


static void put_dtd (dTD_t * dtd)
{
    dtd->next = dtd_free_list;
    dtd_free_list = dtd;
}


static void schedule_dtd (int ep, dTD_t * dtd)
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

    if (qh->first == NULL) {
        qh->first = dtd;
        qh->last = dtd;
    }
    else
        qh->last->next = dtd;

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
        ser_w_string ("Oops, EPST\r\n");
}


static bool schedule_buffer (int ep, const void * data, unsigned length)
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
    dtd->buffer_page[4] = (0xfffff000 & (unsigned) data) + 16384;

    schedule_dtd (ep, dtd);
    return true;
}


void respond_to_setup (unsigned ep, unsigned setup1,
                       const void * descriptor, unsigned length)
{
    if ((setup1 >> 16) < length)
        length = setup1 >> 16;

    // The DMA won't take this into account...
    /* if (descriptor && (unsigned) descriptor < 0x10000000) */
    /*     descriptor += * M4MEMMAP; */

    if (!schedule_buffer (ep + 0x80, descriptor, length))
        return;                         // Bugger.

    if (*ENDPTSETUPSTAT & (1 << ep))
        ser_w_string ("Oops, EPSS\r\n");

    if (length == 0)
        return;                         // No data so no ack...

    // Now the status dtd...
    if (!schedule_buffer (ep, NULL, 0))
        return;

    if (*ENDPTSETUPSTAT & (1 << ep))
        ser_w_string ("Oops, EPSS\r\n");

    /* for (int i = 0 ; i != length; ++i) */
    /*     ser_w_hex (((unsigned char *) descriptor)[i], 2, " "); */
    /* ser_w_string ("\r\n"); */
}


static dTD_t * retire_dtd (dTD_t * d, dQH_t * qh)
{
    dTD_t * next = d->next;
    put_dtd (d);
    if (next == NULL || next == (dTD_t*) 1) {
        next = NULL;
        qh->last = NULL;
    }

    qh->first = next;
    return next;
}

typedef void retire_function_t (int ep, dQH_t * qh, dTD_t * dtd);

static void endpt_complete (int ep, dQH_t * qh, retire_function_t * cb)
{
    // Clean-up the DTDs...
    if (qh->first == NULL)
        return;

    // Just clear any success...
    dTD_t * d = qh->first;
    while (!(d->length_and_status & 0x80)) {
        ser_w_hex (d->length_and_status, 8, " ok length and status\r\n");
        if (cb)
            cb (ep, qh, d);
        d = retire_dtd (d, qh);
        if (d == NULL)
            return;
    }

    if (!(d->length_and_status & 0x7f))
        return;                         // Still going.

    // FIXME - what do we actually want to do on errors?
    ser_w_hex (d->length_and_status, 8, " ERROR length and status\r\n");
    if (cb)
        cb (ep, qh, d);
    if (retire_dtd (d, qh))
        *ENDPTPRIME = ep;               // Reprime the endpoint.
}


static void start_mgmt (void)
{
    if (*ENDPTCTRL1 & 0x800000)
        return;                         // Already running.

    ser_w_string ("Starting mgmt...\r\n");

    // FIXME - 81 length?
    QH[1].IN.capabilities = 0x20040000;
    QH[1].IN.next = (dTD_t*) 1;
    // FIXME - default mgmt packets?
    *ENDPTCTRL1 = 0xcc0000;

    // Default mgmt packets.
    schedule_buffer (0x81, network_connected, sizeof network_connected);
    schedule_buffer (0x81, speed_notification100, sizeof speed_notification100);
}


static void start_network (void)
{
    if (*ENDPTCTRL2 & 0x80)
        return;                         // Already running.

    ser_w_string ("Starting network...\r\n");

    QH[2].IN.capabilities = 0x02000000;
    QH[2].IN.next = (dTD_t*) 1;
    QH[2].OUT.capabilities = 0x02000000;
    QH[2].OUT.next = (dTD_t*) 1;

    *ENDPTCTRL2 = 0x00c800c8;

    for (int i = 0; i != 3; ++i)
        schedule_buffer (2, (void *) tx_ring_buffer + 4096 * i, 0x0800);
    // FIXME - setup ethernet.
}


static void stop_network (void)
{
    if (!(*ENDPTCTRL2 & 0x80))
        return                          // Already stopped.

    ser_w_string ("Stopping network...\r\n");

    do {
        *ENDPTFLUSH = 0x40004;
        while (*ENDPTFLUSH & 0x40004);
    }
    while (*ENDPTSTAT & 0x40004);
    *ENDPTCTRL2 = 0;
    // Cleanup any dtds.  FIXME - fix buffer handling.
    endpt_complete (0, &QH[2].OUT, NULL);
    endpt_complete (0, &QH[2].IN, NULL);
    // FIXME - stop ethernet.
}


static void stop_mgmt (void)
{
    stop_network();

    if (!(*ENDPTCTRL1 & 0x800000))
        return;                         // Already stopped.

    *ENDPTCTRL1 = 0;
    do {
        *ENDPTFLUSH = 0x20000;
        while (*ENDPTFLUSH & 0x20000);
    }
    while (*ENDPTSTAT & 0x20000);
    // Cleanup any dtds.  FIXME - fix buffer handling.
    endpt_complete (0, &QH[1].IN, NULL);
}


// FIXME - we probably only want EP 0.
static void process_setup (int i)
{
    static unsigned zero = 0;
    qh_pair_t * qh = &QH[i];

    *ENDPTCOMPLETE = 0x10001 << i;
    unsigned setup0;
    unsigned setup1;
    do {
        *USBCMD |= 1 << 13;             // Set tripwire.
        setup0 = qh->OUT.setup0;
        setup1 = qh->OUT.setup1;
    }
    while (!(*USBCMD & (1 << 13)));
    *USBCMD &= ~(1 << 13);
    while (*ENDPTSETUPSTAT & (1 << i))
        *ENDPTSETUPSTAT = 1 << i;

    // FIXME - flush old setups.

    switch (setup0 & 0xffff) {
    case 0x0080:                        // Get status.
        respond_to_setup (i, setup1, &zero, 2);
        break;
    // case 0x0100:                        // Clear feature device.
    //     break;
    // case 0x0101:                        // Clear feature interface.
    //     break;
    // case 0x0102:                        // Clear feature endpoint.
    //     break;
    // case 0x0880:                        // Get configuration.
    //     break;
    case 0x0680:                        // Get descriptor.
        switch (setup0 >> 24) {
        case 1:                         // Device.
            respond_to_setup (i, setup1, device_descriptor,
                              DEVICE_DESCRIPTOR_SIZE);
            break;
        case 2:                         // Configuration.
            respond_to_setup (i, setup1, config_descriptor,
                              CONFIG_DESCRIPTOR_SIZE);
            break;
        // case 6:                         // Device qualifier.
        //     respond_to_setup (i, setup1, qualifier_descriptor,
        //                       QUALIFIER_DESCRIPTOR_SIZE);
        //     break;
        case 3: {                        // String.
            unsigned index = (setup0 >> 16) & 255;
            if (index < sizeof string_descriptors / 4) {
                respond_to_setup (i, setup1,
                                  string_descriptors[index],
                                  *string_descriptors[index]);
                break;
            }
            // FALL THROUGH.
        }
        case 7:                         // Other speed config.
        default:
            *ENDPTCTRL0 = 0x810081;     // Stall....
            ser_w_string ("STALL on get descriptor.\r\n");
            break;
        }
        break;
    // case 0x0a81:                        // Get interface.
    //     break;
    case 0x0500:                        // Set address.
        if (((setup0 >> 16) & 127) == 0)
            stop_mgmt();                // Stop everything if back to address 0.
        *DEVICEADDR = ((setup0 >> 16) << 25) | (1 << 24);
        respond_to_setup (i, setup1, NULL, 0);
        break;
    case 0x0900:                        // Set configuration.
        // This leaves us in the default alternative, so always stop the
        // network.
        stop_network();
        start_mgmt();
        respond_to_setup (i, setup1, NULL, 0);
        break;
    // case 0x0700:                        // Set descriptor.
    //     break;
    // case 0x0300:                        // Set feature device.
    //     break;
    // case 0x0301:                        // Set feature interface.
    //     break;
    // case 0x0302:                        // Set feature endpoint.
    //     break;
    case 0x0b01:                        // Set interface.
        if ((setup1 & 0xffff) == 1) {
            if (setup0 & 0xffff0000)
                start_network();
            else
                stop_network();
        }
        respond_to_setup (i, setup1, NULL, 0);
        break;
    // case 0x0c82:                        // Synch frame.
    //     break;
    // case 0x2140:                        // Set ethernet multicast.
    //     break;
    // case 0x2141:                        // Set eth. power mgmt filter.
    //     break;
    // case 0x2142:                        // Get eth. power mgmt filter.
    //     break;
    case 0x2143:                        // Set eth. packet filter.
        // Just fake it for now...
        respond_to_setup (i, setup1, NULL, 0);
        break;
    // case 0x2144:                        // Get eth. statistic.
    //     break;
    default:
        *ENDPTCTRL0 = 0x810081;         // Stall....
        ser_w_string ("STALL on setup request.\r\n");
        break;
    }

    ser_w_hex (setup0, 8, " setup0\r\n");
    ser_w_hex (setup1, 8, " setup1\r\n\r\n");
}



static void endpt_mgmt_complete (int ep, dQH_t * qh, dTD_t * dtd)
{
    // Retire the buffer also...
}


static void endpt_tx_complete (int ep, dQH_t * qh, dTD_t * dtd)
{
    // Send the buffer off to the network...
    // For now, flip the ethertype bits and bounce the packet back.
    // FIXME - overflow on full packet....
    unsigned buffer = dtd->buffer_page[0];
    unsigned char * p = (unsigned char *) (buffer & 0xfffff800);
    p[12] ^= 255;
    p[13] ^= 255;
    schedule_buffer (0x82, p, buffer & 0x7ff);
    ser_w_hex (buffer, 8, "tx complete.\r\n");
}


static void endpt_rx_complete (int ep, dQH_t * qh, dTD_t * dtd)
{
    // Re-queue the buffer for network data.
    // For now, push it back onto the tx queue.
    schedule_buffer (2, (unsigned char *) (dtd->buffer_page[0] & 0xfffff000),
                     0x0800);
    ser_w_string ("rx complete.\r\n");
}


void doit (void)
{
//    RESET_CTRL[0] = 1 << 17;
#if 0
    *PLL0USB_CTRL = 0x0100081c;         // Off, direct in, direct out.
    // Configure the clock to USB.
    // Generate 480MHz off IRC...
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    * (v32*) 0x40050020 = 0x01000818;   // Control.
    * (v32*) 0x40050024 = 0x06167ffa;   // mdiv
    * (v32*) 0x40050028 = 0x00302062;   // np_div.
    *PLL0USB_CTRL = 0x0100081d;         // On, direct in, direct out.

    while (!(PLL0USB_STAT & 1));
#endif

    dtd_free_list = NULL;
    for (int i = 0; i != NUM_DTDS; ++i) {
        ser_w_hex ((unsigned) &DTD[i], 8, " is a DTD\r\n");
        put_dtd (&DTD[i]);
    }

    for (int i = 0; i != 6; ++i) {
        ser_w_hex ((unsigned) &QH[i].OUT, 8, " OUT\r\n");
        ser_w_hex ((unsigned) &QH[i].IN, 8, " IN\r\n\r\n");
        QH[i].OUT.first = NULL;
        QH[i].OUT.last = NULL;
        QH[i].IN.first = NULL;
        QH[i].IN.last = NULL;
    }

    *USBCMD = 2;                        // Reset.
    while (*USBCMD & 2);

    *USBINTR = 0;
    *USBMODE = 0xa;                     // Device.  Tripwire.
    *OTGSC = 9;
    //*PORTSC1 = 0x01000000;              // Only full speed for now.

    QH[0].OUT.capabilities = 0x20408000;
    QH[0].OUT.current = (void *) 1;

    QH[0].IN.capabilities = 0x20408000;
    QH[0].IN.current = (void *) 1;

    // Set the endpoint list pointer.
    *ENDPOINTLISTADDR = (unsigned) &QH;
    *DEVICEADDR = 0;

    *USBCMD = 1;                        // Run.

    while (1) {
        if (*USBSTS & 0x40) {
            *ENDPTNAK = 0xffffffff;
            *ENDPTNAKEN = 1;
            *USBSTS = 0xffffffff;
            *ENDPTSETUPSTAT = *ENDPTSETUPSTAT;
            while (*ENDPTPRIME);
            *ENDPTFLUSH = 0xffffffff;
            if (!(*PORTSC1 & 0x100))
                ser_w_string ("Bugger\r\n");
            *ENDPTCTRL0 = 0x00c000c0; // Bit 6 and 22 are undoc...
            *DEVICEADDR = 0;
            //while (*USBSTS & 0x40);
            // FIXME - stop network if running.
            ser_w_string ("Reset processed...\r\n");
        }

        // Check for setup on 0.  FIXME - will other set-ups interrupt?
        if (*ENDPTSETUPSTAT & 1) {
            *ENDPTSETUPSTAT = 1;
            process_setup (0);
        }

        unsigned complete = *ENDPTCOMPLETE;
        *ENDPTCOMPLETE = complete;

        if (complete & 4)
            endpt_complete (4, &QH[2].OUT, endpt_tx_complete);
        if (complete & 0x40000)
            endpt_complete (0x40000, &QH[2].IN, endpt_rx_complete);
        if (complete & 0x20000)
            endpt_complete (0x20000, &QH[1].IN, endpt_mgmt_complete);

        if (complete & 1)
            endpt_complete(1, &QH[0].OUT, NULL);
        if (complete & 0x10000)
            endpt_complete(0x10000, &QH[0].IN, NULL);
    }
}

void * start[64] __attribute__ ((section (".start")));
void * start[64] = {
    (void*) 0x10089ff0,
    doit
};
