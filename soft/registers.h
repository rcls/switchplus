#ifndef REGISTERS_H_
#define REGISTERS_H_

#define __memory_barrier() asm volatile ("" : : : "memory")
#define __interrupt_disable() asm volatile ("cpsid i\n" ::: "memory");
#define __interrupt_enable() asm volatile ("cpsie i\n" ::: "memory");
#define __interrupt_wait() asm volatile ("wfi\n");
#define __interrupt_wait_go() do {              \
        __interrupt_wait();                     \
        __interrupt_enable();                   \
        __interrupt_disable();                  \
    } while (0)

#define __section(s) __attribute__ ((section (s)))
#define __aligned(s) __attribute__ ((aligned (s)))

static inline void spin_for(unsigned n)
{
    for (unsigned i = 0; i != n; ++i)
        asm volatile("");
}

typedef volatile unsigned char v8;
typedef volatile unsigned v32;
typedef v8 v8_32[32];
typedef v32 v32_32[32];

// Ethernet DMA descriptor (short form).
typedef struct EDMA_DESC_t {
    unsigned status;
    unsigned count;
    void * buffer1;
    void * buffer2;
} EDMA_DESC_t;

#define GPIO 0x400F4000

#define GPIO_BYTE ((v8_32 *) GPIO)
#define GPIO_WORD ((v32_32 *) (GPIO + 0x1000))
#define GPIO_DIR ((v32 *) (GPIO + 0x2000))

#define USART3_THR ((v32 *) 0x400c2000)
#define USART3_RBR ((v32 *) 0x400c2000)
#define USART3_DLL ((v32 *) 0x400c2000)
#define USART3_DLM ((v32 *) 0x400c2004)
#define USART3_IER ((v32 *) 0x400c2004)
#define USART3_FCR ((v32 *) 0x400c2008)
#define USART3_LCR ((v32 *) 0x400c200c)
#define USART3_LSR ((v32 *) 0x400c2014)
#define USART3_SCR ((v32 *) 0x400c201c)
#define USART3_ACR ((v32 *) 0x400c2020)
#define USART3_ICR ((v32 *) 0x400c2024)
#define USART3_FDR ((v32 *) 0x400c2028)
#define USART3_OSR ((v32 *) 0x400c202c)
#define USART3_HDEN ((v32 *) 0x400c2040)

#define CGU 0x40050000

#define FREQ_MON ((v32 *) (CGU + 0x14))

// Not all PLLs have all registers...
typedef struct PLL_t {
    unsigned stat;
    unsigned ctrl;
    unsigned mdiv;
    unsigned np_div;
    unsigned frac;
} PLL_t;

#define PLL0USB ((volatile PLL_t *) 0x4005001c)
#define PLL0AUDIO ((volatile PLL_t *) 0x4005002c)
#define PLL1 ((volatile PLL_t *) 0x40050040)

#define IDIVA_CTRL ((v32*) (CGU + 0x48))
#define IDIVC_CTRL ((v32*) (CGU + 0x50))
#define BASE_M4_CLK ((v32*) (CGU + 0x6c))

#define BASE_PHY_RX_CLK ((v32 *) (CGU + 0x78))
#define BASE_PHY_TX_CLK ((v32 *) (CGU + 0x7c))
#define BASE_LCD_CLK ((v32 *) (CGU + 0x88))
#define BASE_SSP0_CLK ((v32 *) (CGU + 0x94))
#define BASE_SSP1_CLK ((v32 *) (CGU + 0x98))
#define BASE_UART3_CLK ((v32 *) (CGU + 0xa8))

#define CCU1 ((v32 *) 0x40051000)
#define CCU2 ((v32 *) 0x40052000)

#define SCU 0x40086000

#define SFSP ((v32_32 *) SCU)
#define SFSCLK ((v32 *) (SCU + 0xc00))
#define EMCDELAYCLK ((v32*) 0x40086d00)

#define OTP ((const unsigned *) 0x40045000)

typedef struct ssp_t {
    unsigned cr0;
    unsigned cr1;
    unsigned dr;
    unsigned sr;
    unsigned cpsr;
    unsigned imsc;
    unsigned ris;
    unsigned mis;
    unsigned icr;
    unsigned dmacr;
} ssp_t;

#define SSP0 ((volatile ssp_t *) 0x40083000)
#define SSP1 ((volatile ssp_t *) 0x400c5000)

#define RESET_CTRL ((v32 *) 0x40053100)
#define RESET_ACTIVE_STATUS ((v32 *) 0x40053150)

#define CREG0 ((v32 *) 0x40043004)
#define M4MEMMAP ((v32 *) 0x40043100)
#define FLASHCFGA ((v32 *) 0x40043120)
#define FLASHCFGB ((v32 *) 0x40043124)
#define CREG6 ((v32 *) 0x4004312c)

typedef struct control_regs1_t {        // 0x400431xx only.
    void * m4memmap;
    unsigned dummy[5];
    unsigned creg5;
    unsigned dmamux;
    unsigned flashcfga;
    unsigned flashcfgb;
    unsigned etb_cfg;
    unsigned creg6;
    unsigned m4txevent;
} control_regs1_t;
#define CREG100 ((volatile control_regs1_t *) 0x40043100)

typedef struct mac_t {
    unsigned config;
    unsigned frame_filter;
    unsigned hashtable_high;
    unsigned hashtable_low;
    unsigned mii_addr;
    unsigned mii_data;
    unsigned flow_ctrl;
    unsigned vlan_tag;
    unsigned dummy1;
    const unsigned debug;
    unsigned rwake_frflt;
    unsigned pmt_ctrl_stat;
    unsigned dummy2;
    unsigned dummy3;
    const unsigned intr;
    unsigned intr_mask;
    unsigned addr0_high;
    unsigned addr0_low;
} mac_t;

#define MAC ((volatile mac_t *) 0x40010000)

_Static_assert(sizeof(mac_t) == 0x48, "mac size");

#define MAC_TIMESTP_CTRL ((v32 *) (0x40010700))

typedef struct edma_t {
    unsigned bus_mode;
    unsigned trans_poll_demand;
    unsigned rec_poll_demand;
    volatile void * rec_des_addr;
    volatile void * trans_des_addr;
    unsigned stat;
    unsigned op_mode;
    unsigned int_en;
    unsigned mfrm_bufof;
    unsigned rec_int_wdt;

    unsigned dummy[8];

    unsigned curhost_trans_des;
    unsigned curhost_rec_des;
    unsigned curhost_trans_buf;
    unsigned curhost_recbuf;
} edma_t;

_Static_assert(sizeof(edma_t) == 0x58, "edma size");

#define EDMA ((volatile edma_t *) 0x40011000)

typedef struct usb_t {
    unsigned cmd;
    unsigned sts;
    unsigned intr;
    unsigned frindex;
    unsigned dummy;
    union {
        unsigned device_addr;
        unsigned periodic_list_base;
    };
    union {
        void * endpoint_list_addr;
        void * async_list_addr;
    };
    unsigned tt_ctrl;
    unsigned burst_size;
    unsigned tx_fill_tuning;
    unsigned dummy2[3];
    unsigned binterval;
    unsigned endpt_nak;
    unsigned endpt_nak_en;
    unsigned dummy3;
    unsigned portsc1;
} usb_t;

#define USB ((volatile usb_t *) 0x40006140)

_Static_assert((unsigned) &USB->portsc1 == 0x40006184, "portsc1");

typedef struct usb_endpoints_t {
    unsigned dummy;
    unsigned otgsc;
    unsigned usbmode;

    unsigned setupstat;
    unsigned prime;
    unsigned flush;
    unsigned stat;
    unsigned complete;
    unsigned ctrl[6];
} usb_endpoints_t;
_Static_assert(sizeof(usb_endpoints_t) == 0x38, "Usb endpoint size");

#define ENDPT ((volatile usb_endpoints_t *) 0x400061a0)


#define NVIC ((v32*) 0xE000E000)
#define NVIC_ISER (NVIC + 64)
#define NVIC_ICER (NVIC + 96)
#define NVIC_ISPR (NVIC + 128)
#define NVIC_ICPR (NVIC + 160)
#define NVIC_IABR (NVIC + 192)
#define NVIC_IPR ((v8*) (NVIC + 256))
#define NVIC_STIR (NVIC + 960)

typedef struct system_control_block_t {
    const unsigned cpuid;
    unsigned icsr;
    unsigned vtor;
    unsigned aircr;
    unsigned scr;
    unsigned ccr;
    unsigned shpr1;
    unsigned shpr2;
    unsigned shpr3;
    unsigned shcsr;
    unsigned cfsr;
    unsigned hfsr;
    unsigned dfsr;
    unsigned mmfar;
    unsigned bfar;
    unsigned afsr;
    const unsigned cpuid_scheme[16];
    unsigned dummy[2];
    unsigned cpacr;
} system_control_block_t;

#define SCB ((volatile system_control_block_t *) 0xe00ed00)
_Static_assert((unsigned) &SCB->mmfar == 0xe00ed34, "SCB MMFAR");
_Static_assert((unsigned) &SCB->cpacr == 0xe00ed88, "SCB CPACR");

typedef struct EMC_t {
    unsigned emc_control;
    unsigned status;
    unsigned config;
    unsigned _dummy1[5];

    unsigned dynamic_control;
    unsigned refresh;
    unsigned read_config;
    unsigned _dummy2;

    unsigned rp;
    unsigned ras;
    unsigned srex;
    unsigned apr;
    unsigned dal;
    unsigned wr;
    unsigned rc;
    unsigned rfc;
    unsigned xsr;
    unsigned rrd;
    unsigned mrd;

    unsigned _dummy3[41];

    struct {
        unsigned config;
        unsigned rascas;
        unsigned dummy[6];
    } port[4];
} EMC_t;

_Static_assert(__builtin_offsetof(EMC_t, rp) == 48, "rp offset");
_Static_assert(__builtin_offsetof(EMC_t, port) == 256, "port0 offset");

#define EMC ((volatile EMC_t *) 0x40005000)

typedef struct lcd_t {
    unsigned timh;
    unsigned timv;
    unsigned pol;
    unsigned le;
    const void * upbase;
    const void * lpbase;
    unsigned ctrl;
    unsigned intmsk;
    unsigned intraw;
    unsigned intstat;
    unsigned intclr;
    unsigned upcurr;
    unsigned lpcurr;
} lcd_t;
#define LCD ((volatile lcd_t *) 0x40008000)

#define LCD_PAL ((v32*) 0x400008200)

#define CRSR_IMG ((v32*) 0x40008800)

typedef struct cursor_t {
    unsigned ctrl;
    unsigned cfg;
    unsigned pal0;
    unsigned pal1;
    unsigned xy;
    unsigned clip;
    unsigned dummy[2];
    unsigned intmsk;
    unsigned intclr;
    unsigned intraw;
    unsigned intstat;
} cursor_t;
#define CRSR ((volatile cursor_t *) 0x40008c00)

typedef struct i2c_t {
    unsigned conset;
    unsigned stat;
    unsigned dat;
    unsigned adr0;
    unsigned sclh;
    unsigned scll;
    unsigned conclr;
    unsigned mmctl;
    unsigned adr1;
    unsigned adr2;
    unsigned adr3;
    unsigned data_buffer;
    unsigned mask0;
    unsigned mask1;
    unsigned mask2;
    unsigned mask3;
} i2c_t;

#define I2C0 ((volatile i2c_t *) 0x400a1000)

typedef struct gpdma_channel_t {
    const volatile void * srcaddr;
    volatile void * destaddr;
    void * lli;
    unsigned control;
    unsigned config;
    unsigned dummy[3];
} gpdma_channel_t;
_Static_assert(sizeof(gpdma_channel_t) == 32, "gpdma_channel size");

typedef struct gpdma_t {
    const unsigned intstat;
    const unsigned inttcstat;
    unsigned inttcclear;
    const unsigned interrstat;
    unsigned interrclr;
    const unsigned rawinttcstat;
    const unsigned rawinterrstat;
    const unsigned enbldchns;
    unsigned softbreq;
    unsigned softsreq;
    unsigned softlbreq;
    unsigned softlsreq;
    unsigned config;
    unsigned sync;
    unsigned dummy[50];

    gpdma_channel_t channel[8];
} gpdma_t;
_Static_assert(sizeof(gpdma_t) == 512, "gpdma size");

#define GPDMA ((volatile gpdma_t *) 0x40002000)

typedef struct event_router_t {
    unsigned hilo;
    unsigned edge;
    unsigned dummy[0xfd0 / 4];
    unsigned clr_en;
    unsigned set_en;
    const unsigned status;
    const unsigned enable;
    unsigned clr_stat;
    unsigned set_stat;
} event_router_t;

_Static_assert(sizeof(event_router_t) == 0xff0, "event_router_t size");

#define EVENT_ROUTER ((volatile event_router_t *) 0x40044000)

// M4 interrupt numbers.
enum {
    m4_dma      = 2,
    m4_ethernet = 5,
    m4_lcd      = 7,
    m4_usb0     = 8,
    m4_switch   = 42,                   // Actually the event-router.
    m4_num_interrupts
};

#endif
