#ifndef REGISTERS_H_
#define REGISTERS_H_

typedef volatile unsigned char v8;
typedef volatile unsigned v32;
typedef v8 v8_32[32];
typedef v32 v32_32[32];

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


#define SCU 0x40086000

#define SFSP ((v32_32 *) SCU)

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

#endif
