#ifndef REGISTERS_H_
#define REGISTERS_H_

#define __memory_barrier() asm volatile ("" : : : "memory")
#define __interrupt_disable() asm volatile ("cpsid i\n" ::: "memory");
#define __interrupt_enable() asm volatile ("cpsie i\n" ::: "memory");
#define __interrupt_wait() asm volatile ("wfi\n");

#define __section(s) __attribute__ ((section (s)))
#define __aligned(s) __attribute__ ((aligned (s)))

typedef volatile unsigned char v8;
typedef volatile unsigned v32;
typedef v8 v8_32[32];
typedef v32 v32_32[32];

typedef struct dTD_t dTD_t;

typedef void dtd_completion_t (dTD_t * dtd);

// USB transfer descriptor.
struct dTD_t {
    struct dTD_t * volatile next;
    volatile unsigned length_and_status;
    volatile unsigned buffer_page[5];

    dtd_completion_t * completion;      // For our use...
};


// Ethernet DMA descriptor (short form).
typedef struct EDMA_DESC_t {
    unsigned status;
    unsigned count;
    void * buffer1;
    void * buffer2;
} EDMA_DESC_t;

#define GPIO 0x400F4000

#define GPIO_BYTE ((v8_32 *) GPIO)
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

#define PLL0USB_STAT ((v32 *) (CGU + 0x1c))
#define PLL0USB_CTRL ((v32 *) (CGU + 0x20))
#define PLL0USB_MDIV ((v32 *) (CGU + 0x24))
#define PLL0USB_NP_DIV ((v32 *) (CGU + 0x28))

#define PLL0AUDIO_STAT ((v32 *) (CGU + 0x2c))
#define PLL0AUDIO_CTRL ((v32 *) (CGU + 0x30))
#define PLL0AUDIO_MDIV ((v32 *) (CGU + 0x34))
#define PLL0AUDIO_NP_DIV ((v32 *) (CGU + 0x38))
#define PLL0AUDIO_FRAC ((v32 *) (CGU + 0x3c))

#define PLL1_STAT ((v32*) (CGU + 0x40))
#define PLL1_CTRL ((v32*) (CGU + 0x44))

#define IDIVA_CTRL ((v32*) (CGU + 0x48))
#define IDIVC_CTRL ((v32*) (CGU + 0x50))
#define BASE_M4_CLK ((v32*) (CGU + 0x6c))

#define BASE_PHY_RX_CLK ((v32 *) (CGU + 0x78))
#define BASE_PHY_TX_CLK ((v32 *) (CGU + 0x7c))
#define BASE_LCD_CLK ((v32 *) (CGU + 0x88))
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

#define ENET 0x40010000

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
    unsigned rec_des_addr;
    unsigned trans_des_addr;
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

#define USB0 0x40006000

#define CAPLENGTH ((v32*) (USB0 + 0x100))
#define HCSPARAMS ((v32*) (USB0 + 0x104))
#define HCCPARAMS ((v32*) (USB0 + 0x108))
#define DCIVERSION ((v32*) (USB0 + 0x120))
#define DCCPARAMS ((v32*) (USB0 + 0x124))

#define USBCMD ((v32*) (USB0 + 0x140))
#define USBSTS ((v32*) (USB0 + 0x144))
#define USBINTR ((v32*) (USB0 + 0x148))
#define FRINDEX ((v32*) (USB0 + 0x14c))
#define DEVICEADDR ((v32*) (USB0 + 0x154))
#define PERIODICLISTBASE ((v32*) (USB0 + 0x154))
#define ENDPOINTLISTADDR ((v32*) (USB0 + 0x158))
#define ASYNCLISTADDR ((v32*) (USB0 + 0x158))
#define TTCTRL ((v32*) (USB0 + 0x15c))
#define BURSTSIZE ((v32*) (USB0 + 0x160))
#define TXFILLTUNING ((v32*) (USB0 + 0x164))
#define BINTERVAL ((v32*) (USB0 + 0x174))
#define ENDPTNAK ((v32*) (USB0 + 0x178))
#define ENDPTNAKEN ((v32*) (USB0 + 0x17c))
#define PORTSC1 ((v32*) (USB0 + 0x184))
#define OTGSC ((v32*) (USB0 + 0x1a4))
#define USBMODE ((v32*) (USB0 + 0x1a8))

#define ENDPTSETUPSTAT ((v32*) (USB0 + 0x1ac))
#define ENDPTPRIME ((v32*) (USB0 + 0x1b0))
#define ENDPTFLUSH ((v32*) (USB0 + 0x1b4))
#define ENDPTSTAT ((v32*) (USB0 + 0x1b8))
#define ENDPTCOMPLETE ((v32*) (USB0 + 0x1bc))
#define ENDPTCTRL0 ((v32*) (USB0 + 0x1c0))
#define ENDPTCTRL1 ((v32*) (USB0 + 0x1c4))
#define ENDPTCTRL2 ((v32*) (USB0 + 0x1c8))
#define ENDPTCTRL3 ((v32*) (USB0 + 0x1cc))
#define ENDPTCTRL4 ((v32*) (USB0 + 0x1d0))
#define ENDPTCTRL5 ((v32*) (USB0 + 0x1d4))

#define NVIC ((v32*) 0xE000E000)
#define NVIC_ISER (NVIC + 64)
#define NVIC_ICER (NVIC + 96)
#define NVIC_ISPR (NVIC + 128)
#define NVIC_ICPR (NVIC + 160)
#define NVIC_IABR (NVIC + 192)
#define NVIC_IPR (NVIC + 256)
#define NVIC_STIR (NVIC + 960)

#define CORTEX_M_AIRCR ((v32*) 0xe000ed0c)

#define EMC ((v32*) 0x40005000)

#define EMCCONTROL (EMC)
#define EMCSTATUS (EMC + 1)
#define EMCCONFIG (EMC + 2)

#define DYNAMICCONTROL (EMC + 8)
#define DYNAMICREFRESH (EMC + 9)
#define DYNAMICREADCONFIG (EMC + 10)

#define DYNAMICRP   (EMC + 12)
#define DYNAMICRAS  (EMC + 13)
#define DYNAMICSREX (EMC + 14)
#define DYNAMICAPR  (EMC + 15)
#define DYNAMICDAL  (EMC + 16)
#define DYNAMICWR   (EMC + 17)
#define DYNAMICRC   (EMC + 18)
#define DYNAMICRFC  (EMC + 19)
#define DYNAMICXSR  (EMC + 20)
#define DYNAMICRRD  (EMC + 21)
#define DYNAMICMRD  (EMC + 22)

#define DYNAMICCONFIG0 (EMC + 64)
#define DYNAMICRASCAS0 (EMC + 65)

#define DYNAMICCONFIG1 (EMC + 72)
#define DYNAMICRASCAS1 (EMC + 73)

#define DYNAMICCONFIG2 (EMC + 80)
#define DYNAMICRASCAS2 (EMC + 81)

#define DYNAMICCONFIG3 (EMC + 88)
#define DYNAMICRASCAS3 (EMC + 89)

#define LCD_BASE ((v32*) 0x40008000)

typedef struct lcd_t {
    unsigned timh;
    unsigned timv;
    unsigned pol;
    unsigned le;
    unsigned upbase;
    unsigned lpbase;
    unsigned ctrl;
    unsigned intmsk;
    unsigned intraw;
    unsigned intstat;
    unsigned intclr;
    unsigned upcurr;
    unsigned lpcurr;
} lcd_t;
#define LCD ((volatile lcd_t *) 0x40008000)

#define LCD_PAL (LCD_BASE + 128)

#define CRSR_IMG (LCD_BASE + 512)

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

#endif
