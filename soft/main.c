// Lets try to bring up a usb device...

#include "callback.h"
#include "freq.h"
#include "jtag.h"
#include "lcd.h"
#include "monkey.h"
#include "registers.h"
#include "sdram.h"
#include "spirom.h"
#include "square.h"
#include "switch.h"
#include "usb.h"

#include <stdbool.h>
#include <stddef.h>

// Buffer space for each packet transfer.
#define BUF_SIZE 2048

#define EDMA_COUNT 4
#define EDMA_MASK 3
static volatile EDMA_DESC_t tx_dma[EDMA_COUNT] __section ("ahb0.tx_dma");
static volatile EDMA_DESC_t rx_dma[EDMA_COUNT] __section ("ahb0.rx_dma");
static unsigned tx_dma_retire;
static unsigned tx_dma_insert;
static unsigned rx_dma_retire;
static unsigned rx_dma_insert;

// While the USB is idle, keep a linked list of the idle tx buffers.
static void * idle_tx_buffers;

extern unsigned char bss_start;
extern unsigned char bss_end;

enum string_descs_t {
    sd_lang,
    sd_ralph,
    sd_switch,
    sd_0001,
    sd_eth_addr,
    sd_eth_mgmt,
    sd_eth_idle,
    sd_eth_showtime,
    sd_monkey,
    sd_dfu,
};

static void endpt_tx_complete (dTD_t * dtd);


// String0 - lang. descs.
static const unsigned short string_lang[2] = u"\x0304\x0409";
static const unsigned short string_ralph[6] = u"\x030c""Ralph";
static const unsigned short string_switch[7] = u"\x030e""Switch";
static const unsigned short string_0001[5] = u"\x030a""0001";
static const unsigned short string_eth_addr[13] = u"\x031a""42baffc46ee9";
static const unsigned short string_eth_mgmt[20] =
    u"\x0328""Ethernet Management";
static const unsigned short string_eth_idle[14] = u"\x031c""Ethernet Idle";
static const unsigned short string_eth_showtime[18] =
    u"\x0324""Ethernet Showtime";
static const unsigned short string_monkey[7] = u"\x030e""Monkey";
static const unsigned short string_dfu[4] = u"\x0308""DFU";

static const unsigned short * const string_descriptors[] = {
    string_lang,
    [sd_ralph] = string_ralph,
    [sd_switch] = string_switch,
    [sd_0001] = string_0001,
    [sd_eth_addr] = string_eth_addr,
    [sd_eth_mgmt] = string_eth_mgmt,
    [sd_eth_idle] = string_eth_idle,
    [sd_eth_showtime] = string_eth_showtime,
    [sd_monkey] = string_monkey,
    [sd_dfu] = string_dfu,
};


#define DEVICE_DESCRIPTOR_SIZE 18
static const unsigned char device_descriptor[] = {
    DEVICE_DESCRIPTOR_SIZE,
    1,                                  // type: device
    0, 2,                               // bcdUSB.
    0,                                  // class - compound.
    0,                                  // subclass.
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
_Static_assert (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor),
                "device_descriptor size");

enum usb_interfaces_t {
    usb_intf_eth_comm,
    usb_intf_eth_data,
    usb_intf_monkey,
    usb_intf_dfu,
    usb_num_intf
};


#define CONFIG_DESCRIPTOR_SIZE (9 + 9 + 5 + 13 + 5 + 7 + 9 + 9 + 7 + 7 + 9 + 7 + 7 + 9 + 7)
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
    sd_eth_addr,                        // Mac address string.
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
    // Interface (monkey).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_monkey,                    // interface number.
    0,                                  // alternate setting.
    2,                                  // number of endpoints.
    0xff,                               // interface class (vendor specific).
    'S',                                // interface sub-class.
    'S',                                // protocol.
    sd_monkey,                          // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    3,                                  // OUT 3.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x83,                               // IN 3.
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
_Static_assert (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor),
                "config_descriptor size");


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
_Static_assert (QUALIFIER_DESCRIPTOR_SIZE == sizeof (qualifier_descriptor),
                "qualifier_descriptor size");

static unsigned char rx_ring_buffer[8192] __aligned (4096) __section ("ahb1");
static unsigned char tx_ring_buffer[8192] __aligned (4096) __section ("ahb2");

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
    // M4 BUS, GPIO, LCD, Ethernet, USB0, EMC, DMA, M4 Core, EMC DIV,
    // Flash A, Flash B,
    CCU1_VALID_4 & ~0x3837d,
    // SSP0, SCU, CREG
    CCU1_VALID_5 & ~0xc8,
    // USART3, SSP1
    CCU1_VALID_6 & ~0x24,
    // Periph... (?)
    CCU1_VALID_7 & ~0x0,
};

// USB1, SPI, VADC, APLL, USART2, UART1, USART0, SDIO.
#define CCU_BASE_DISABLE 0x013a0e00

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
    for (unsigned mask = CCU_BASE_DISABLE; mask; mask >>= 1, base += 64)
        if ((mask & 1) && (*base & 1))
            *base = 0;
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

    schedule_buffer (ep + 0x80, descriptor, length,
                     length == 0 ? callback : NULL);

    if (*ENDPTSETUPSTAT & (1 << ep))
        puts ("Oops, EPSS\n");

    if (length == 0)
        return;                         // No data so no ack...

    // Now the status dtd...
    schedule_buffer (ep, NULL, 0, callback);

    if (*ENDPTSETUPSTAT & (1 << ep))
        puts ("Oops, EPSS\n");
}


static void enter_dfu (void)
{
    bool was_empty = monkey_is_empty();
    verbose ("Enter DFU\n");
    if (was_empty && log_monkey)
        for (int i = 0; !monkey_is_empty() && i != 1000000; ++i);

    NVIC_ICER[0] = 0xffffffff;

    // Switch back to IDIVC.
    *BASE_M4_CLK = 0x0e000800;

    *USBCMD = 2;                       // Reset USB.
    while (*USBCMD & 2);

    unsigned fakeotp[64];
    for (int i = 0; i != 64; ++i)
        fakeotp[i] = OTP[i];

    fakeotp[12] = (6 << 25) + (1 << 23); // Boot mode; use custom usb ids.
    fakeotp[13] = 0x524cf055;           // Product id / vendor id.

    typedef unsigned F (void*);
    ((F*) 0x1040158d) (fakeotp);

    puts ("Bugger.\n");
    while (1);
}


static void initiate_enter_dfu (dTD_t * dtd)
{
    callback_simple (enter_dfu);
}


static void serial_byte (unsigned byte)
{
    switch (byte) {
    case 'd':
        debug_flag = !debug_flag;
        puts (debug_flag ? "Debug on\n" : "Debug off\n");
        return;
    case 'e':
        mii_report();
        return;
    case 'f':
        clock_report();
        return;
    case 'h': case '?':
        puts("<d>ebug, <f>req, <j>tag, <l>cd, <m>emtest, <p>retty, <s>pi\n"
             "<r>eset, <R>eboot, df<u>, <v>erbose\n");
        return;
    case 'j':
        jtag_cmd();
        return;
    case 'l':
        lcd_init();
        return;
    case 'm':
        memtest();
        return;
    case 'p':
        square_interact();
        return;
    case 's':
        spirom_command();
        return;
    case 'r':
        puts ("Reset!\n");
        *BASE_M4_CLK = 0x0e000800;      // Back to IDIVC.
        for (int i = 0; i != 100000; ++i)
            asm volatile ("");
        while (1)
            *CORTEX_M_AIRCR = 0x05fa0004;
    case 'R':
        puts ("Cold Reboot!\n");
        *BASE_M4_CLK = 0x0e000800;      // Back to IDIVC.
        for (int i = 0; i != 100000; ++i)
            asm volatile ("");
        __interrupt_disable();
        while (1) {
            RESET_CTRL[1] = 0xffffffff;
            RESET_CTRL[0] = 0xffffffff;
        }
    case 'u':
        enter_dfu();
        return;
    case 'v':
        verbose_flag = !verbose_flag;
        puts (verbose_flag ? "Verbose ON\n" : "Verbose OFF\n");
        return;
    }

    // Don't echo escape...
    if (byte == 27)
        return;

    if (byte == '\r')
        putchar ('\n');
    else
        putchar (byte);
}


static void start_mgmt (void)
{
    if (*ENDPTCTRL1 & 0x800000)
        return;                         // Already running.

    puts ("Starting mgmt...\n");

    qh_init (0x81, 0x20400000);
    // FIXME - default mgmt packets?
    *ENDPTCTRL1 = 0xcc0000;

    // No 0-size-frame on the monkey.
    qh_init (0x03, 0x22000000);
    qh_init (0x83, 0x22000000);
    *ENDPTCTRL3 = 0x00c800c8;

    init_monkey_usb();
}


static void reuse_tx_buffer(void * buffer)
{
    if (*ENDPTCTRL2 & 0x80)
        schedule_buffer (0x02, buffer, BUF_SIZE, endpt_tx_complete);
    else {
        * (void **) buffer = idle_tx_buffers;
        idle_tx_buffers = buffer;
    }
}


// Completion called for buffers from the OUT endpoint.  Dispatch to the
// ethernet.
static void endpt_tx_complete (dTD_t * dtd)
{
    void * buffer = (void *) dtd->buffer_page[4];
    unsigned status = dtd->length_and_status;
    if (status & 0xff) {                // Errored...
        reuse_tx_buffer(buffer);
        return;
    }

    volatile EDMA_DESC_t * t = &tx_dma[tx_dma_insert++ & EDMA_MASK];

    t->count = BUF_SIZE - (status >> 16);
    t->buffer1 = buffer;
    t->buffer2 = NULL;

    if (tx_dma_insert & EDMA_MASK)
        t->status = 0xf0000000;
    else
        t->status = 0xf0200000;

    EDMA->trans_poll_demand = 0;

    debugf ("TX to MAC..: %08x %08x\n", dtd->buffer_page[0], status);
}


static void notify_network_up (dTD_t * dtd)
{
    schedule_buffer (0x81, network_connected, sizeof network_connected, NULL);
    schedule_buffer (0x81, speed_notification100, sizeof speed_notification100,
                     NULL);
}


static void start_network (void)
{
    if (*ENDPTCTRL2 & 0x80)
        return;                         // Already running.

    puts ("Starting network...\n");

    qh_init (0x02, 0x02000000);
    qh_init (0x82, 0x02000000);

    *ENDPTCTRL2 = 0x00880088;

    while (idle_tx_buffers) {
        void * buffer = idle_tx_buffers;
        idle_tx_buffers = * (void **) buffer;
        schedule_buffer (2, buffer, BUF_SIZE, endpt_tx_complete);
    }
}


static void stop_network (void)
{
    if (!(*ENDPTCTRL2 & 0x80))
        return;                         // Already stopped.

    puts ("Stopping network...\n");

    do {
        *ENDPTFLUSH = 0x40004;
        while (*ENDPTFLUSH & 0x40004);
    }
    while (*ENDPTSTAT & 0x40004);
    *ENDPTCTRL2 = 0;
    // Cleanup any dtds.  FIXME - fix buffer handling.
    endpt_complete (0x02, false);
    endpt_complete (0x82, false);
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
    endpt_complete (0x81, false);
}


static void process_setup (int i)
{
    unsigned long long setup = get_0_setup();
    unsigned setup0 = setup;
    unsigned setup1 = setup >> 32;

    const void * response_data = NULL;
    unsigned response_length = 0xffffffff;
    dtd_completion_t * callback = NULL;

    switch (setup0 & 0xffff) {
    case 0x0021:                        // DFU detach.
        response_length = 0;
        callback = initiate_enter_dfu;
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
                response_length = *string_descriptors[index] & 0xff;
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

    if (response_length != 0xffffffff) {
        respond_to_setup (i, setup1,
                          response_data, response_length, callback);
        debugf ("Setup: %08x %08x\n", setup0, setup1);
    }
    else {
        *ENDPTCTRL0 = 0x810081;         // Stall....
        printf ("Setup STALL: %08x %08x\n", setup0, setup1);
    }
}


static void queue_rx_dma(void * buffer)
{
    volatile EDMA_DESC_t * r = &rx_dma[rx_dma_insert++ & EDMA_MASK];

    if (rx_dma_insert & EDMA_MASK)
        r->count = BUF_SIZE;
    else
        r->count = 0x8000 + BUF_SIZE;
    r->buffer1 = buffer;
    r->buffer2 = 0;

    r->status = 0x80000000;

    EDMA->rec_poll_demand = 0;
}


static void endpt_rx_complete (dTD_t * dtd)
{
    // Re-queue the buffer for network data.
    unsigned buffer = dtd->buffer_page[4];

    queue_rx_dma((void *) buffer);

    debugf("RX complete: %08x %08x\n",
           dtd->buffer_page[0], dtd->length_and_status);
}


static void retire_rx_dma (volatile EDMA_DESC_t * rx)
{
    // FIXME - handle errors.
    // FIXME - handle overlength packets.
    if (!(*ENDPTCTRL2 & 0x80)) {        // If usb not running, put back to eth.
        queue_rx_dma(rx->buffer1);
        return;
    }

    // Give the buffer to USB.
    unsigned status = rx->status;
    void * buffer = rx->buffer1;
    schedule_buffer (0x82, buffer, (status >> 16) & 0x7ff,
                     endpt_rx_complete);
    debugf ("RX to usb..: %p %08x\n", buffer, status);
}


// Handle a completed TX entry from ethernet.
static void retire_tx_dma (volatile EDMA_DESC_t * tx)
{
    // FIXME - handle errors.
    // Give the buffer to USB...
    void * buffer = tx->buffer1;
    reuse_tx_buffer(tx->buffer1);

    debugf ("TX Complete: %p %08x\n", buffer, tx->status);
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

    EDMA->bus_mode = 1;                 // Reset ethernet DMA.
    while (EDMA->bus_mode & 1);

    MAC->addr0_low = 0xc4ffba42;
    MAC->addr0_high = 0x8000e96e;

    // Set filtering options.  Promiscuous / recv all.
    MAC->frame_filter = 0x80000001;

    MAC->config = 0xc900;

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

    EDMA->trans_des_addr = (unsigned) tx_dma;
    EDMA->rec_des_addr = (unsigned) rx_dma;

    // Start ethernet & it's dma.
    MAC->config = 0xc90c;
    EDMA->op_mode = 0x2002;
}


static void usb_interrupt (void)
{
    unsigned status = *USBSTS;
    *USBSTS = status;                   // Clear interrupts.

    unsigned complete = *ENDPTCOMPLETE;
    *ENDPTCOMPLETE = complete;

    // Don't log interrupts that look like they're monkey completions.
    if (debug_flag && (!log_monkey || (complete != 0x80000)))
        puts ("usb interrupt...\n");

    if (complete & 4)
        endpt_complete (2, true);
    if (complete & 0x40000)
        endpt_complete (0x82, true);
    if (complete & 0x20000)
        endpt_complete (0x81, true);

    if (complete & 1)
        endpt_complete (0, true);
    if (complete & 0x10000)
        endpt_complete (0x80, true);

    if (complete & 8)
        endpt_complete (0x03, true);
    if (complete & 0x80000)
        endpt_complete (0x83, true);

    // Check for setup on 0.  FIXME - will other set-ups interrupt?
    unsigned setup = *ENDPTSETUPSTAT;
    *ENDPTSETUPSTAT = setup;
    if (setup & 1)
        process_setup (0);

    // Reset.
    if (status & 0x40) {
        while (*ENDPTPRIME);
        *ENDPTFLUSH = 0xffffffff;

        if (!(*PORTSC1 & 0x100))
            puts ("BuggeR\n");

        stop_mgmt();

        *ENDPTNAK = 0xffffffff;
        *ENDPTNAKEN = 1;

        *ENDPTCTRL0 = 0x00c000c0;
        *DEVICEADDR = 0;
        puts ("USB Reset processed...\n");
    }
}


static void eth_interrupt (void)
{
    debugf ("eth interrupt...\n");
    EDMA->stat = 0x1ffff;               // Clear interrupts.

    while (rx_dma_retire != rx_dma_insert
           && !(rx_dma[rx_dma_retire & EDMA_MASK].status & 0x80000000))
        retire_rx_dma (&rx_dma[rx_dma_retire++ & EDMA_MASK]);

    while (tx_dma_retire != tx_dma_insert
           && !(tx_dma[tx_dma_retire & EDMA_MASK].status & 0x80000000))
        retire_tx_dma (&tx_dma[tx_dma_retire++ & EDMA_MASK]);
}


void main (void)
{
    // Disable all interrupts for now...
    __interrupt_disable();
    NVIC_ICER[0] = 0xffffffff;
    NVIC_ICER[1] = 0xffffffff;

    __memory_barrier();

    for (unsigned char * p = &bss_start; p != &bss_end; ++p)
        *p = 0;

    __memory_barrier();

    init_switch();
    init_ethernet();
    init_monkey_ssp();

    puts ("***********************************\n");
    puts ("**          Supa Switch          **\n");
    puts ("***********************************\n");

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

    // Resets don't always seem to restore PLL1 & BASE_M4_CLK.  Do that now.
    *BASE_M4_CLK = 0x01000800;          // Switch to irc for a bit.
    *PLL1_CTRL = 0x01170940;
    *IDIVC_CTRL = 0x09000808;
    // Wait for locks.
    while (!(*PLL0USB_STAT & 1));
    while (!(*PLL1_STAT & 1));
    // Get DIVC to output PLL1 / 3.
    // Switch back to IDIVC.
    *BASE_M4_CLK = 0x0e000800;

    // Set the flash access time for 160MHz.
    *FLASHCFGA = 0x8000703a;
    *FLASHCFGB = 0x8000703a;

    // Now ramp to 160MHz.
    *IDIVA_CTRL = 0x07000808;
    *BASE_M4_CLK = 0x0c000800;

    disable_clocks();

    // Build the linked list of idle tx buffers.
    idle_tx_buffers = NULL;
    for (int i = 0; i != 4; ++i) {
        void * buffer = tx_ring_buffer + BUF_SIZE * i;
        * (void **) buffer = idle_tx_buffers;
        idle_tx_buffers = buffer;
    }

    usb_init();

    // Enable the ethernet, usb and dma interrupts.
    NVIC_ISER[0] = 0x00000124;
    *USBINTR = 0x00000041;              // Port change, reset, data.
    EDMA->stat = 0x1ffff;
    EDMA->int_en = 0x0001ffff;
    __interrupt_enable();

    lcd_init();

    square_draw9();

    while (true)
        serial_byte (getchar());
}


void * start[64] __attribute__ ((section (".start"), externally_visible));
void * start[64] = {
    [0] = (void*) 0x10089ff0,
    [1] = main,

    [18] = gpdma_interrupt,
    [21] = eth_interrupt,
    [23] = lcd_interrupt,
    [24] = usb_interrupt,
};
