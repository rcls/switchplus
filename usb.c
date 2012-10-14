// Lets try bring ub a usb device...

#include "registers.h"

#include <stdbool.h>
#include <stddef.h>

#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)
#define STATIC_ASSERT(b) extern int JOIN(_sa_dummy_, __LINE__)[b ? 1 : -1]

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

static struct qh_and_dtd_t {
    qh_pair_t QH[6];
#define NUM_DTDS 40
    dTD_t DTD[NUM_DTDS];
} qh_and_dtd __attribute__ ((aligned (2048)));
#define QH qh_and_dtd.QH
#define DTD qh_and_dtd.DTD


typedef struct EDMA_DESC_t {
    unsigned status;
    unsigned count;
    void * buffer1;
    void * buffer2;
} EDMA_DESC_t;


typedef void dtd_completion_t (int ep, dQH_t * qh, dTD_t * dtd);

// This is slightly less than 2k so we can always mask off the size...
#define BUF_SIZE 0x7f0

#define EDMA_COUNT 4
#define EDMA_MASK 3
static volatile EDMA_DESC_t tx_dma[EDMA_COUNT];
static volatile EDMA_DESC_t rx_dma[EDMA_COUNT];
static unsigned tx_dma_retire;
static unsigned tx_dma_insert;
static unsigned rx_dma_retire;
static unsigned rx_dma_insert;

static bool debug;
extern unsigned char bss_start;
extern unsigned char bss_end;

static dTD_t * dtd_free_list;

enum string_descs_t {
    sd_lang,
    sd_ralph,
    sd_switch,
    sd_0001,
    sd_424242424242,
    sd_eth_mgmt,
    sd_eth_idle,
    sd_eth_showtime,
    sd_monkey,
    sd_dfu,
};

// String0 - lang. descs.
static const unsigned char string_lang[] = {
    4, 3, 9, 4
};
static const unsigned char string_ralph[] = {
    12, 3, 'R', 0, 'a', 0, 'l', 0, 'p', 0, 'h', 0
};
static const unsigned char string_switch[] = {
    14, 3, 'S', 0, 'w', 0, 'i', 0, 't', 0, 'c', 0, 'h', 0
};
static const unsigned char string_0001[] = {
    10, 3, '0', 0, '0', 0, '0', 0, '1', 0
};
static const unsigned char string_424242424242[] = {
    26, 3,
    '4', 0, '2', 0, '4', 0, '2', 0, '4', 0, '2', 0,
    '4', 0, '2', 0, '4', 0, '2', 0, '4', 0, '2', 0
};
static const unsigned char string_eth_mgmt[] = {
    40, 3, 'E',0, 't',0, 'h',0, 'e',0, 'r',0, 'n',0, 'e',0, 't',0, ' ',0,
    'M',0, 'a',0, 'n',0, 'a',0, 'g',0, 'e',0, 'm',0, 'e',0, 'n',0, 't',0,
};
static const unsigned char string_eth_idle[] = {
    28, 3, 'E', 0, 't', 0, 'h', 0, 'e', 0, 'r', 0, 'n', 0, 'e', 0, 't', 0,
    ' ', 0, 'I', 0, 'd', 0, 'l', 0, 'e', 0,
};
static const unsigned char string_eth_showtime[] = {
    36, 3, 'E', 0, 't', 0, 'h', 0, 'e', 0, 'r', 0, 'n', 0, 'e', 0, 't', 0,
    ' ', 0, 'S', 0, 'h', 0, 'o', 0, 'w', 0, 't', 0, 'i', 0, 'm', 0, 'e', 0,
};
static const unsigned char string_monkey[] = {
    14, 3, 'M', 0, 'o', 0, 'n', 0, 'k', 0, 'e', 0, 'y', 0
};
static const unsigned char string_dfu[] = {
    8, 3, 'D', 0, 'F', 0, 'U', 0
};
static const unsigned char * const string_descriptors[] = {
    string_lang,
    [sd_ralph] = string_ralph,
    [sd_switch] = string_switch,
    [sd_0001] = string_0001,
    [sd_424242424242] = string_424242424242,
    [sd_eth_mgmt] = string_eth_mgmt,
    [sd_eth_idle] = string_eth_idle,
    [sd_eth_showtime] = string_eth_showtime,
    [sd_monkey] = string_monkey,
    [sd_dfu] = string_dfu,
};


#define DEVICE_DESCRIPTOR_SIZE 18
static const unsigned char device_descriptor[] = {
    DEVICE_DESCRIPTOR_SIZE,
    1,                                  // type:
    0, 2,                               // bcdUSB.
    2,                                  // class - CDC.
    6,                                  // subclass.
    0,                                  // protocol.
    64,                                 // Max packet size.
    0x55, 0xf0,                         // Vendor-ID.
    'L', 'R',                           // Device-ID.
    0x34, 0x12,                         // Revision number.
    sd_ralph,                           // Manufacturer string index.
    sd_switch,                          // Product string index.
    sd_0001,                            // Serial number string index.
    1                                   // Number of configurations.
    };
STATIC_ASSERT (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor));

enum usb_interfaces_t {
    usb_intf_eth_comm,
    usb_intf_eth_data,
    usb_intf_dfu,
    usb_num_intf
};


#define CONFIG_DESCRIPTOR_SIZE (9 + 9 + 5 + 13 + 5 + 7 + 9 + 9 + 7 + 7 + 9 + 7)
static const unsigned char config_descriptor[] = {
    // Config.
    9,                                  // length.
    2,                                  // type: config.
    CONFIG_DESCRIPTOR_SIZE & 0xff,      // size.
    CONFIG_DESCRIPTOR_SIZE >> 8,
    usb_num_intf,                       // num interfaces.
    1,                                  // configuration number.
    0,                                  // string descriptor index.
    0x80,                               // attributes, not self powered.
    250,                                // current (500mA).
    // Interface (comm).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_eth_comm,                  // interface number.
    0,                                  // alternate setting.
    1,                                  // number of endpoints.
    2,                                  // interface class.
    6,                                  // interface sub-class.
    0,                                  // protocol.
    sd_eth_mgmt,                        // interface string index.
    // CDC header...
    5,                                  // length
    0x24,                               // CS_INTERFACE
    0,                                  // subtype = header
    0x10, 1,                            // Version number 1.10
    // Ethernet functional descriptor.
    13,
    0x24,                               // cs_interface,
    15,                                 // ethernet
    sd_424242424242,                    // Mac address string.
    0, 0, 0, 0,                         // Statistics bitmap.
    0, 7,                               // Max segment size.
    0, 0,                               // Multicast filters.
    0,                                  // Number of power filters.
    // Union...
    5,                                  // Length
    0x24,                               // CS_INTERFACE
    6,                                  // Union
    usb_intf_eth_comm,                  // Interface 0 is control.
    usb_intf_eth_data,                  // Interface 1 is sub-ordinate.
    // Endpoint.
    7,
    5,
    0x81,                               // IN 1
    3,                                  // Interrupt
    64, 0,                              // 64 bytes
    11,                                 // binterval
    // Interface (data).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_eth_data,                  // interface number.
    0,                                  // alternate setting.
    0,                                  // number of endpoints.
    10,                                 // interface class (data).
    6,                                  // interface sub-class.
    0,                                  // protocol.
    sd_eth_idle,                        // interface string index.
    // Interface (data).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_eth_data,                  // interface number.
    1,                                  // alternate setting.
    2,                                  // number of endpoints.
    10,                                 // interface class (data).
    6,                                  // interface sub-class.
    0,                                  // protocol.
    sd_eth_showtime,                    // interface string index.
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
    0,
    // Interface (DFU).
    9,                                  // Length.
    4,                                  // Type = Interface.
    usb_intf_dfu,                       // Interface number.
    0,                                  // Alternate.
    0,                                  // Num. endpoints.
    0xfe, 1, 0,                         // Application specific; DFU.
    sd_dfu,                             // Name...
    // Function (DFU)
    7,                                  // Length.
    0x21,                               // Type.
    9,                                  // Will detach.  Download only.
    0, 255,                             // Detach timeout.
    0, 16,                              // Transfer size.
};
STATIC_ASSERT (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor));


static const unsigned char network_connected[] = {
    0xa1, 0, 0, 1, 0, 0, 0, 0
};
// static const unsigned char network_disconnected[] = {
//     0xa1, 0, 0, 0, 0, 0, 0, 0
// };

static const unsigned char speed_notification100[] = {
    0xa1, 0x2a, 0, 0, 0, 1, 0, 8,
    0, 0xe1, 0xf5, 0x05, 0, 0xe1, 0xf5, 0x05,
};


#define QUALIFIER_DESCRIPTOR_SIZE 10
const unsigned char qualifier_descriptor[] = {
    QUALIFIER_DESCRIPTOR_SIZE,          // Length.
    6,                                  // Type
    0, 2,                               // usb version
    255, 1, 1,                          // class / subclass / protocol
    64, 1, 0                            // packet size / num. configs / zero
};
STATIC_ASSERT (QUALIFIER_DESCRIPTOR_SIZE == sizeof (qualifier_descriptor));


static unsigned char rx_ring_buffer[8192] __attribute__ ((aligned (2048)));
static unsigned char tx_ring_buffer[8192] __attribute__ ((aligned (2048)));

static volatile bool enter_dfu;

static void ser_w_byte (unsigned byte)
{
    while (!(*USART3_LSR & 32));         // Wait for THR to be empty.
    *USART3_THR = byte;
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
        ser_w_byte ("0123456789abcdef"[(value >> (i * 4)) & 15]);
    }
    ser_w_string (term);
}


#define CCU1_VALID_0 0
#define CCU1_VALID_1 0x3f
#define CCU1_VALID_2 0x1f
#define CCU1_VALID_3 1
#define CCU1_VALID_4 (0x1c0000 + 0x3e3ff)
#define CCU1_VALID_5 0xff
#define CCU1_VALID_6 0xff
#define CCU1_VALID_7 0xf

static const unsigned ccu1_disable_mask[] = {
    0,
    CCU1_VALID_1 & ~0x1,
    CCU1_VALID_2 & ~0x1,
    CCU1_VALID_3 & ~0x0,
    // M4 BUS, GPIO, Ethernet, USB0, DMA(?), M4 Core, Flash A, Flash B,
    CCU1_VALID_4 & ~0x30335,
    // SSP0, SCU, CREG
    CCU1_VALID_5 & ~0xc8,
    // USART3
    CCU1_VALID_6 & ~0x4,
    // Periph... (?)
    CCU1_VALID_7 & ~0x0,
};

// USB1, SPI, VADC, APLL, USART2, UART1, USART0, SSP1, SDIO.
#define CCU_BASE_DISABLE 0x017a0e00

static void disable_clocks(void)
{
    for (unsigned i = 0; i < 8; ++i) {
        volatile unsigned * base = (volatile unsigned *) (0x40051000 + i * 256);
        unsigned mask = ccu1_disable_mask[i];
        do {
            if ((mask & 1) && (*base & 1))
                *base = 0;

            mask >>= 1;
            base += 2;
        }
        while (mask);
    }

    volatile unsigned * base = (volatile unsigned *) 0x40051000;
    for (unsigned mask = CCU_BASE_DISABLE; mask;
         mask >>= 1, base += 64)
        if ((mask & 1) && (*base & 1))
            *base = 0;
}


static dTD_t * get_dtd (void)
{
    dTD_t * r = dtd_free_list;
    if (r == NULL) {
        ser_w_string ("Out of DTDs!!!\r\n");
        ser_w_hex (tx_dma_retire, 8, " ");
        ser_w_hex (tx_dma_insert, 8, " tx retire, insert\r\n");
        ser_w_hex (rx_dma_retire, 8, " ");
        ser_w_hex (rx_dma_insert, 8, " rx retire, insert\r\n");
        while (1);
    }
    dtd_free_list = r->next;
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
        ser_w_string ("Oops, EPST\r\n");
}


static bool schedule_buffer (int ep, const void * data, unsigned length,
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
    dtd->buffer_page[4] = (0xfffff000 & (unsigned) data) + 16384;
    dtd->dummy = (unsigned) cb;

    schedule_dtd (ep, dtd);
    return true;
}


static void respond_to_setup (unsigned ep, unsigned setup1,
                              const void * descriptor, unsigned length,
                              dtd_completion_t * callback)
{
    if ((setup1 >> 16) < length)
        length = setup1 >> 16;

    // The DMA won't take this into account...
    /* if (descriptor && (unsigned) descriptor < 0x10000000) */
    /*     descriptor += * M4MEMMAP; */

    if (!schedule_buffer (ep + 0x80, descriptor, length,
                          length == 0 ? callback : NULL))
        return;                         // Bugger.

    if (*ENDPTSETUPSTAT & (1 << ep))
        ser_w_string ("Oops, EPSS\r\n");

    if (length == 0)
        return;                         // No data so no ack...

    // Now the status dtd...
    if (!schedule_buffer (ep, NULL, 0, callback))
        return;

    if (*ENDPTSETUPSTAT & (1 << ep))
        ser_w_string ("Oops, EPSS\r\n");
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


static void endpt_complete (int ep, dQH_t * qh)
{
    // Clean-up the DTDs...
    if (qh->first == NULL)
        return;

    // Successes...
    dTD_t * d = qh->first;
    while (!(d->length_and_status & 0x80)) {
        //ser_w_hex (d->length_and_status, 8, " ok length and status\r\n");
        if (d->dummy)
            ((dtd_completion_t *) d->dummy) (ep, qh, d);
        d = retire_dtd (d, qh);
        if (d == NULL)
            return;
    }

    if (!(d->length_and_status & 0x7f))
        return;                         // Still going.

    // FIXME - what do we actually want to do on errors?
    ser_w_hex (d->length_and_status, 8, " ERROR length and status\r\n");
    if (d->dummy)
        ((dtd_completion_t *) d->dummy) (ep, qh, d);
    if (retire_dtd (d, qh))
        *ENDPTPRIME = ep;               // Reprime the endpoint.
}


static void start_mgmt (void)
{
    if (*ENDPTCTRL1 & 0x800000)
        return;                         // Already running.

    ser_w_string ("Starting mgmt...\r\n");

    // FIXME - 81 length?
    QH[1].IN.capabilities = 0x20400000;
    QH[1].IN.next = (dTD_t*) 1;
    // FIXME - default mgmt packets?
    *ENDPTCTRL1 = 0xcc0000;
}


static void endpt_tx_complete (int ep, dQH_t * qh, dTD_t * dtd)
{
    // Send the buffer off to the network...
    unsigned buffer = dtd->buffer_page[0];
    unsigned status = dtd->length_and_status;
    // FIXME - detect errors and oversize packets.

    volatile EDMA_DESC_t * t = &tx_dma[tx_dma_insert++ & EDMA_MASK];

    t->count = buffer & 0x7ff;
    t->buffer1 = (void *) (buffer & 0xfffff800);
    t->buffer2 = NULL;

    if (tx_dma_insert & EDMA_MASK)
        t->status = 0xf0000000;
    else
        t->status = 0xf0200000;

    *EDMA_TRANS_POLL_DEMAND = 0;

    if (debug) {
        ser_w_hex (buffer, 8, " ");
        ser_w_hex (status, 8, " tx to MAC.\r\n");
    }
}


static void notify_network_up (int ep, dQH_t * qh, dTD_t * dtd)
{
    schedule_buffer (0x81, network_connected, sizeof network_connected, NULL);
    schedule_buffer (0x81, speed_notification100, sizeof speed_notification100,
                     NULL);
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

    *ENDPTCTRL2 = 0x00880088;

    // Start ethernet & it's dma.
    *MAC_CONFIG = 0xc90c;
    *EDMA_OP_MODE = 0x2002;

    for (int i = 0; i != 4; ++i)
        schedule_buffer (2, (void *) tx_ring_buffer + 2048 * i, BUF_SIZE,
                         endpt_tx_complete);
}


static void stop_network (void)
{
    if (!(*ENDPTCTRL2 & 0x80))
        return                          // Already stopped.

    ser_w_string ("Stopping network...\r\n");

    // Stop ethernet & it's dma.
    *EDMA_OP_MODE = 0;
    *MAC_CONFIG = 0xc900;

    do {
        *ENDPTFLUSH = 0x40004;
        while (*ENDPTFLUSH & 0x40004);
    }
    while (*ENDPTSTAT & 0x40004);
    *ENDPTCTRL2 = 0;
    // Cleanup any dtds.  FIXME - fix buffer handling.
    endpt_complete (0, &QH[2].OUT);
    endpt_complete (0, &QH[2].IN);
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
    endpt_complete (0, &QH[1].IN);
}


static void process_setup (int i)
{
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

    const void * response_data = NULL;
    unsigned response_length = 0xffffffff;
    dtd_completion_t * callback = NULL;

    switch (setup0 & 0xffff) {
    case 0x0021:                        // DFU detach.
        response_length = 0;
        enter_dfu = true;
        break;
    case 0x0080:                        // Get status.
        response_data = "\0";
        response_length = 2;
        break;
    case 0x03a1: {                      // DFU status.
        static const unsigned char status[] = { 0, 100, 0, 0, 0, 0 };
        response_data = status;
        response_length = 6;
        break;
    }
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
            response_data = device_descriptor;
            response_length = DEVICE_DESCRIPTOR_SIZE;
            break;
        case 2:                         // Configuration.
            response_data = config_descriptor;
            response_length = CONFIG_DESCRIPTOR_SIZE;
            break;
        case 6:                         // Device qualifier.
            response_data = qualifier_descriptor;
            response_length = QUALIFIER_DESCRIPTOR_SIZE;
            break;
        case 3: {                        // String.
            unsigned index = (setup0 >> 16) & 255;
            if (index < sizeof string_descriptors / 4) {
                response_data = string_descriptors[index];
                response_length = *string_descriptors[index];
            }
            break;
        }
        case 7:                         // Other speed config.
        default:
            ;
        }
        break;
    // case 0x0a81:                        // Get interface.
    //     break;
    case 0x0500:                        // Set address.
        if (((setup0 >> 16) & 127) == 0)
            stop_mgmt();                // Stop everything if back to address 0.
        *DEVICEADDR = ((setup0 >> 16) << 25) | (1 << 24);
        response_length = 0;
        break;
    case 0x0900:                        // Set configuration.
        // This leaves us in the default alternative, so always stop the
        // network.
        stop_network();
        start_mgmt();
        response_length = 0;
        callback = notify_network_up;
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
        switch (setup1 & 0xffff) {
        case 1:                         // Interface 1 (data)
            if ((setup1 & 0xffff) == usb_intf_eth_data) {
                if (setup0 & 0xffff0000) {
                    start_network();
                    callback = notify_network_up;
                }
                else
                    stop_network();
            }
            response_length = 0;
            break;
        case usb_intf_eth_comm:         // Interface 0 (comms) - ignore.
        case usb_intf_dfu:                         // Interface 2 (DFU).
            response_length = 0;
            break;
        }
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
        response_length = 0;
        break;
    // case 0x2144:                        // Get eth. statistic.
    //     break;
    default:
        break;
    }

    if (response_length != 0xffffffff)
        respond_to_setup (i, setup1,
                          response_data, response_length, callback);
    else {
        *ENDPTCTRL0 = 0x810081;         // Stall....
        ser_w_string ("STALL on setup request.\r\n");
    }
    // FIXME - flush old setups.
    ser_w_hex (setup0, 8, " ");
    ser_w_hex (setup1, 8, " setup\r\n");
}


static void endpt_rx_complete (int ep, dQH_t * qh, dTD_t * dtd)
{
    // Re-queue the buffer for network data.
    unsigned buffer = dtd->buffer_page[0];
    unsigned status = dtd->length_and_status;

    volatile EDMA_DESC_t * r = &rx_dma[rx_dma_insert++ & EDMA_MASK];

    if (rx_dma_insert & EDMA_MASK)
        r->count = BUF_SIZE;
    else
        r->count = 0x8000 + BUF_SIZE;
    r->buffer1 = (void*) (buffer & 0xfffff800);
    r->buffer2 = 0;

    r->status = 0x80000000;

    *EDMA_REC_POLL_DEMAND = 0;

    if (debug) {
        ser_w_hex (buffer, 8, " ");
        ser_w_hex (status, 8, " rx complete.\r\n");
    }
}


static void retire_rx_dma (volatile EDMA_DESC_t * rx)
{
    // FIXME - handle errors.
    // FIXME - handle overlength packets.
    // Give the buffer to USB.... FIXME: handle shutdown races.
    unsigned status = rx->status;
    void * buffer = rx->buffer1;
    schedule_buffer (0x82, buffer, (status >> 16) & 0x7ff,
                     endpt_rx_complete);
    if (debug) {
        ser_w_hex ((unsigned) buffer, 8, " ");
        ser_w_hex (status, 8, " rx to usb\r\n");
    }
}


static void retire_tx_dma (volatile EDMA_DESC_t * tx)
{
    // FIXME - handle errors.
    // Give the buffer to USB...
    void * buffer = tx->buffer1;
    schedule_buffer (0x02, buffer, 0x7ff, endpt_tx_complete);
    if (debug) {
        ser_w_hex ((unsigned) buffer, 8, " ");
        ser_w_hex (tx->status, 8, " tx complete\r\n");
    }
}


// data is big endian, lsb align.
static unsigned spi_io (unsigned data, int num_bytes)
{
    // Wait for idle.
    while (*SSP0_SR & 16);
    // Clear input FIFO.
    while (*SSP0_SR & 4)
        *SSP0_DR;

    // Take the SSEL GPIO low.
    GPIO_BYTE[7][16] = 0;

    for (int i = 0; i < num_bytes; ++i)
        *SSP0_DR = (data >> (num_bytes - i - 1) * 8) & 255;

    // Wait for idle.
    while (*SSP0_SR & 16);

    // Take the SSEL GPIO high again.
    GPIO_BYTE[7][16] = 1;

    unsigned result = 0;
    for (int i = 0; i < num_bytes; ++i) {
        while (!(*SSP0_SR & 4));
        result = result * 256 + (*SSP0_DR & 255);
    }

    return result;
}

static unsigned spi_reg_read (unsigned address)
{
    return spi_io (0x30000 + ((address & 255) * 256), 3) & 255;
}

static unsigned spi_reg_write (unsigned address, unsigned data)
{
    return spi_io (0x20000 + ((address & 255) * 256) + data, 3) & 255;
}


static void init_switch (void)
{
    // SMRXD0 - ENET_RXD0 - T12 - P1_15
    // SMRXD1 - ENET_RXD1 - L3 - P0_0
    SFSP[0][0] |= 24; // Disable pull-up, enable pull-down.
    SFSP[1][15] |= 24;

    // Switch reset is E16, GPIO7[9], PE_9.
    GPIO_BYTE[7][9] = 0;
    GPIO_DIR[7] |= 1 << 9;
    SFSP[14][9] = 4;                    // GPIO is function 4....
    // Out of reset...
    GPIO_BYTE[7][9] = 1;

    // Wait ~ 100us.
    for (int i = 0; i < 10000; ++i)
        asm volatile ("");

    // Switch SPI is on SSP0.
    // SPIS is SSP0_SSEL on E11, PF_1 - use as GPIO7[16], function 4.
    // SPIC is SSP0_SCK on B14, P3_3, function 2.
    // SPID is SSP0_MOSI on C11, P3_7, function 5.
    // SPIQ is SSP0_MISO on B13, P3_6, function 5.

    // Set SPIS output hi.
    GPIO_BYTE[7][16] = 1;
    GPIO_DIR[7] |= 1 << 16;
    SFSP[15][1] = 4;                    // GPIO is function 4.

    // Set the prescaler to divide by 2.
    *SSP0_CPSR = 2;

    // Is SSP0 unit clock running by default?  "All branch clocks are enabled
    // by default".
    // Keep clock HI while idle, CPOL=1,CPHA=1
    // Output data on falling edge.  Read data on rising edge.
    // Divide clock by 30.
    *SSP0_CR0 = 0x00001dc7;

    // Enable SSP0.
    *SSP0_CR1 = 0x00000002;

    // Set up the pins.
    SFSP[3][3] = 2; // Clock pin, has high drive but we don't want that.
    SFSP[3][7] = 5;
    SFSP[3][6] = 0x45; // Function 5, enable input buffer.

    /* ser_w_hex (spi_reg_read (0), 2, " reg0  "); */
    /* ser_w_hex (spi_reg_read (1), 2, " reg1 "); */
    spi_reg_write (1, 1);         // Start switch.
    /* ser_w_hex (spi_reg_read (1), 2, "\r\n"); */

    ser_w_string ("Switch is running\r\n");
}


static void init_ethernet (void)
{
    SFSP[1][19] = 0xc0;       // TX_CLK (M11, P1_19, func 0) input; no deglitch.
    SFSP[1][17] = 0x63;       // MDIO (M8, P1_17, func 3) input.
    SFSP[1][15] = 0xc3;       // RXD0 (T12, P1_15, func 3) input; no deglitch.
    SFSP[0][0] = 0xc2;        // RXD1 (L3, P0_0, func 2) input; no deglitch.
    SFSP[12][8] = 0xc3;       // RX_DV (N4, PC_8, func 3) input; no deglitch.

    SFSP[12][1] = 3;                    // MDC (E4, PC_1, func 3).
    SFSP[1][18] = 3;                    // TXD0, N12, P1_18, func 3.
    SFSP[1][20] = 3;                    // TXD1, M10, P1_20, func 3.
    SFSP[0][1] = 6;                     // TX_EN, M2, P0_1, func 6.

    // Set the PHY clocks.
    *BASE_PHY_TX_CLK = 0x03000800;
    *BASE_PHY_RX_CLK = 0x03000800;

    *CREG6 = 4;                         // Set ethernet to RMII.

    RESET_CTRL[0] = 1 << 22;            // Reset ethernet.
    while (!(RESET_ACTIVE_STATUS[0] & (1 << 22)));

    *EDMA_BUS_MODE = 1;                 // Reset ethernet DMA.
    while (*EDMA_BUS_MODE & 1);

    *MAC_ADDR0_LOW = 0x4242;
    *MAC_ADDR0_HIGH = 0x42424242;

    // Set-up buffers and write descriptors....
    // *EDMA_REC_DES_ADDR = (unsigned) rdes;
    // *EDMA_TRANS_DES_ADDR = (unsigned) tdes;

    // Set filtering options.  Promiscuous / recv all.
    *MAC_FRAME_FILTER = 0x80000001;

    *MAC_CONFIG = 0xc900;

    // Set-up the dma descs.
    for (int i = 0; i != EDMA_COUNT; ++i) {
        tx_dma[i].status = 0;

        rx_dma[i].status = 0x80000000;
        rx_dma[i].count = BUF_SIZE;     // Status bits?
        rx_dma[i].buffer1 = rx_ring_buffer + 2048 * i;
        rx_dma[i].buffer2 = 0;
    }

    rx_dma[EDMA_MASK].count = 0x8000 + BUF_SIZE; // End of ring.

    rx_dma_insert = EDMA_COUNT;

    *EDMA_TRANS_DES_ADDR = (unsigned) tx_dma;
    *EDMA_REC_DES_ADDR = (unsigned) rx_dma;
    // Set dma recv/xmit bits.
    //*EDMA_OP_MODE = 0x2002;
}


static void start_serial (void)
{
    // Bring up USART3.
    SFSP[2][3] = 2;                     // P2_3, J12 is TXD on function 2.
    SFSP[2][4] = 0x42;                  // P2_4, K11 is RXD on function 2.

    // Set the USART3 clock to be the 12MHz IRC.
    *BASE_UART3_CLK = 0x01000800;

    *USART3_LCR = 0x83;                 // Enable divisor access.

    // From the user-guide: Based on these findings, the suggested USART setup
    // would be: DLM = 0, DLL = 4, DIVADDVAL = 5, and MULVAL = 8.
    *USART3_DLM = 0;
    *USART3_DLL = 4;
    *USART3_FDR = 0x85;

    *USART3_LCR = 0x3;                  // Disable divisor access, 8N1.
    *USART3_FCR = 1;                    // Enable fifos.
    *USART3_IER = 1;                    // Enable receive interrupts.
}


static void usb_interrupt (void)
{
    if (debug)
        ser_w_string ("usb interrupt...\r\n");

    unsigned status = *USBSTS;
    *USBSTS = status;                   // Clear interrupts.

    unsigned complete = *ENDPTCOMPLETE;
    *ENDPTCOMPLETE = complete;

    if (complete & 4)
        endpt_complete (4, &QH[2].OUT);
    if (complete & 0x40000)
        endpt_complete (0x40000, &QH[2].IN);
    if (complete & 0x20000)
        endpt_complete (0x20000, &QH[1].IN);

    if (complete & 1)
        endpt_complete (1, &QH[0].OUT);
    if (complete & 0x10000)
        endpt_complete (0x10000, &QH[0].IN);

    // Check for setup on 0.  FIXME - will other set-ups interrupt?
    unsigned setup = *ENDPTSETUPSTAT;
    *ENDPTSETUPSTAT = setup;
    if (setup & 1)
        process_setup (0);

    // Reset.
    if (status & 0x40) {
        stop_network();
        stop_mgmt();
        *ENDPTNAK = 0xffffffff;
        *ENDPTNAKEN = 1;

        *ENDPTSETUPSTAT = *ENDPTSETUPSTAT;
        while (*ENDPTPRIME);
        *ENDPTFLUSH = 0xffffffff;
        if (!(*PORTSC1 & 0x100))
            ser_w_string ("Bugger\r\n");
        *ENDPTCTRL0 = 0x00c000c0;
        *DEVICEADDR = 0;
        //while (*USBSTS & 0x40);
        // FIXME - stop network if running.
        ser_w_string ("Reset processed...\r\n");
    }
}


static void eth_interrupt (void)
{
    if (debug)
        ser_w_string ("eth interrupt...\r\n");
    *EDMA_STAT = 0x1ffff;               // Clear interrupts.

    while (rx_dma_retire != rx_dma_insert
        && !(rx_dma[rx_dma_retire & EDMA_MASK].status & 0x80000000))
        retire_rx_dma (&rx_dma[rx_dma_retire++ & EDMA_MASK]);

    while (tx_dma_retire != tx_dma_insert
        && !(tx_dma[tx_dma_retire & EDMA_MASK].status & 0x80000000))
        retire_tx_dma (&tx_dma[tx_dma_retire++ & EDMA_MASK]);
}

static void usart3_interrupt (void)
{
    ser_w_string ("usart interrupt\r\n");
    while (*USART3_LSR & 1) {
        switch (*USART3_RBR & 0xff) {
        case 'r':
            ser_w_string ("Reboot!\r\n");
            RESET_CTRL[0] = 0xffffffff;
            break;
        case 'd':
            debug = !debug;
            ser_w_string (debug ? "Debug on\r\n" : "Debug off\r\n");
            break;
        case 'u':
            enter_dfu = 1;
            break;
        }
    }
}


void doit (void)
{
    // Disable all interrupts for now...
    NVIC_ICER[0] = 0xffffffff;
    NVIC_ICER[1] = 0xffffffff;

    __memory_barrier();

    for (unsigned char * p = &bss_start; p != &bss_end; ++p)
        *p = 0;

    __memory_barrier();

//    RESET_CTRL[0] = 1 << 17;
    start_serial();

    init_switch();
    init_ethernet();

    ser_w_string ("Init pll\r\n");

    // 50 MHz in from eth_tx_clk
    // Configure the clock to USB.
    // Generate 480MHz off 50MHz...
    // ndec=5, mdec=32682, pdec=0
    // selr=0, seli=28, selp=13
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    *PLL0USB_CTRL = 0x03000819;
    *PLL0USB_MDIV = (28 << 22) + (13 << 17) + 32682;
    *PLL0USB_NP_DIV = 5 << 12;
    *PLL0USB_CTRL = 0x03000818;         // Divided in, direct out.
    ser_w_string ("Lock wait\r\n");
    while (!(*PLL0USB_STAT & 1));

    disable_clocks();
    ser_w_string ("Clocks disabled.\r\n");

#if 0
    // Now measure the clock rate.
    *FREQ_MON = 0x07800000 + 240;
    while (*FREQ_MON & 0x00800000);
    ser_w_hex (*FREQ_MON, 8, " freq mon\r\n");
#endif

    dtd_free_list = NULL;
    for (int i = 0; i != NUM_DTDS; ++i)
        put_dtd (&DTD[i]);

    for (int i = 0; i != 6; ++i) {
        QH[i].OUT.first = NULL;
        QH[i].OUT.last = NULL;
        QH[i].IN.first = NULL;
        QH[i].IN.last = NULL;
    }

    *USBCMD = 2;                        // Reset.
    while (*USBCMD & 2);

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

    // Enable the ethernet, usb and serial interrupts.
    NVIC_ISER[0] = 0x08000120;
    *USBINTR = 0x00000001;
    *EDMA_STAT = 0x1ffff;
    *EDMA_INT_EN = 0x0001ffff;
    asm volatile ("cpsie if\n");

    do
        asm volatile ("wfi\n" ::: "memory");
    while (!enter_dfu);

    NVIC_ICER[0] = 0xffffffff;

    ser_w_string ("Enter DFU\r\n");

    unsigned fakeotp[64];
    for (int i = 0; i != 64; ++i)
        fakeotp[i] = OTP[i];

    fakeotp[12] = 6 << 25;

    typedef unsigned F (void*);
    ((F*) 0x1040158d) (fakeotp);

    ser_w_string ("Bugger.\r\n");
    while (1);
}


void * start[64] __attribute__ ((section (".start")));
void * start[64] = {
    [0] = (void*) 0x10089ff0,
    [1] = doit,

    [21] = eth_interrupt,
    [24] = usb_interrupt,
    [43] = usart3_interrupt,
};
