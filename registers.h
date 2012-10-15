#ifndef REGISTERS_H_
#define REGISTERS_H_

#define __memory_barrier() asm volatile ("" : : : "memory")

typedef volatile unsigned char v8;
typedef volatile unsigned v32;
typedef v8 v8_32[32];
typedef v32 v32_32[32];

typedef struct dTD_t dTD_t;
typedef struct dQH_t dQH_t;

typedef void dtd_completion_t (int ep, dQH_t * qh, dTD_t * dtd);

struct dTD_t {
    struct dTD_t * volatile next;
    volatile unsigned length_and_status;
    unsigned volatile buffer_page[5];

    dtd_completion_t * completion;      // For our use...
};

struct dQH_t {
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
};

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
#define PLL0USB_STAT ((v32 *) (CGU + 0x1c))
#define PLL0USB_CTRL ((v32 *) (CGU + 0x20))
#define PLL0USB_MDIV ((v32 *) (CGU + 0x24))
#define PLL0USB_NP_DIV ((v32 *) (CGU + 0x28))

#define SCU 0x40086000

#define SFSP ((v32_32 *) SCU)
#define SFSCLK ((v32 *) (SCU + 0xc00))

#define OTP ((const unsigned *) 0x40045000)

#define CGU 0x40050000
#define BASE_PHY_RX_CLK ((v32 *) (CGU + 0x78))
#define BASE_PHY_TX_CLK ((v32 *) (CGU + 0x7c))
#define BASE_UART3_CLK ((v32 *) (CGU + 0xa8))

#define SSP0 0x40083000
#define SSP0_CR0 ((v32 *) SSP0)
#define SSP0_CR1 ((v32 *) (SSP0 + 4))
#define SSP0_DR ((v32 *) (SSP0 + 8))
#define SSP0_SR ((v32 *) (SSP0 + 12))
#define SSP0_CPSR ((v32 *) (SSP0 + 16))
#define SSP0_IMSC ((v32 *) (SSP0 + 20))
#define SSP0_RIS ((v32 *) (SSP0 + 24))
#define SSP0_MIS ((v32 *) (SSP0 + 28))
#define SSP0_ICR ((v32 *) (SSP0 + 32))
#define SSP0_DMACR ((v32 *) (SSP0 + 36))

#define FREQ_MON ((v32 *) 0x40050014)

#define RESET_CTRL ((v32 *) 0x40053100)
#define RESET_ACTIVE_STATUS ((v32 *) 0x40053150)

#define CREG0 ((v32 *) 0x40043004)
#define M4MEMMAP ((v32 *) 0x40043100)
#define CREG6 ((v32 *) 0x4004312c)

#define ENET 0x40010000
#define MAC_CONFIG ((v32 *) ENET)
#define MAC_FRAME_FILTER ((v32 *) (ENET + 4))
#define MAC_HASHTABLE_HIGH ((v32 *) (ENET + 8))
#define MAC_HASHTABLE_LOW ((v32 *) (ENET + 12))
#define MAC_MII_ADDR ((v32 *) (ENET + 16))
#define MAC_MII_DATA ((v32 *) (ENET + 20))
#define MAC_FLOW_CTRL ((v32 *) (ENET + 24))
#define MAC_VLAN_TAG ((v32 *) (ENET + 28))

#define MAC_DEBUG ((v32 *) (ENET + 36))
#define MAC_RWAKE_FRFLT ((v32 *) (ENET + 40))
#define MAC_PMT_CTRL_STAT ((v32 *) (ENET + 44))

#define MAC_INTR ((v32 *) (ENET + 56))
#define MAC_INTR_MASK ((v32 *) (ENET + 60))
#define MAC_ADDR0_HIGH ((v32 *) (ENET + 64))
#define MAC_ADDR0_LOW ((v32 *) (ENET + 68))

#define MAC_TIMESTP_CTRL ((v32 *) (ENET + 0x700))

#define EDMA_BUS_MODE ((v32 *) (ENET + 0x1000))
#define EDMA_TRANS_POLL_DEMAND ((v32 *) (ENET + 0x1000 + 4))
#define EDMA_REC_POLL_DEMAND ((v32 *) (ENET + 0x1000 + 8))
#define EDMA_REC_DES_ADDR ((v32 *) (ENET + 0x1000 + 12))
#define EDMA_TRANS_DES_ADDR ((v32 *) (ENET + 0x1000 + 16))
#define EDMA_STAT ((v32 *) (ENET + 0x1000 + 20))
#define EDMA_OP_MODE ((v32 *) (ENET + 0x1000 + 24))
#define EDMA_INT_EN ((v32 *) (ENET + 0x1000 + 28))
#define EDMA_MFRM_BUFOF ((v32 *) (ENET + 0x1000 + 32))
#define EDMA_REC_INT_WDT ((v32 *) (ENET + 0x1000 + 36))

#define EDMA_CURHOST_TRANS_DES ((v32 *) (ENET + 0x1000 + 72))
#define EDMA_CURHOST_REC_DES ((v32 *) (ENET + 0x1000 + 76))
#define EDMA_CURHOST_TRANS_BUF ((v32 *) (ENET + 0x1000 + 80))
#define EDMA_CURHOST_RECBUF ((v32 *) (ENET + 0x1000 + 84))

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

#endif
