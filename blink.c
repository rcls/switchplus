// We have leds on H5 (P8_1, GPIO4[1]) and K4 (P8_2, GPIO4[2]).

#include <stdint.h>

#define SCU 0x40086000

#define SFSP(A,B) (SCU + A * 128 + B)

#define GPIO 0x400F4000

#define GPIO_BYTE_BASE ((volatile unsigned char *) GPIO)
#define GPIO_WORD_BASE ((volatile uint32_t *) (GPIO + 0x1000))
#define GPIO_BYTE(a,b) (GPIO_BYTE_BASE + (a)*32 + (b))
#define GPIO_WORD(a,b) (GPIO_WORD_BASE + (a)*32 + (b))
#define GB ((volatile unsigned char (*)[32]) GPIO_BYTE_BASE)

#define GPIO_DIR_BASE ((volatile uint32_t *) (GPIO + 0x2000))
#define GPIO_DIR(a) (GPIO_DIR_BASE + (a))

#define GPIO_MASK_BASE ((volatile uint32_t *) (GPIO + 0x2080))
#define GPIO_MASK(a) (GPIO_MASK_BASE + (a))

#define GPIO_PIN_BASE ((volatile uint32_t *) (GPIO + 0x2100))
#define GPIO_PIN(a) (GPIO_MASK_BASE + (a))

#define GPIO_MPIN_BASE ((volatile uint32_t *) (GPIO + 0x2180))
#define GPIO_MPIN(a) (GPIO_MPIN_BASE + (a))

#define GPIO_SET_BASE ((volatile uint32_t *) (GPIO + 0x2200))
#define GPIO_SET(a) (GPIO_SET_BASE + (a))

#define GPIO_CLR_BASE ((volatile uint32_t *) (GPIO + 0x2280))
#define GPIO_CLR(a) (GPIO_CLR_BASE + (a))

#define GPIO_NOT_BASE ((volatile uint32_t *) (GPIO + 0x2300))
#define GPIO_NOT(a) (GPIO_NOT_BASE + (a))

#define UART3_THR ((volatile uint32_t *) 0x400C2000)

void doit (void)
{
    *UART3_THR = 'R';
    *UART3_THR = 'C';
    *UART3_THR = 'L';
    *UART3_THR = '\n';
    *GPIO_BYTE(4,1) = 1;
    *GPIO_BYTE(4,2) = 0;
    *GPIO_DIR(4) |= 6;
    *UART3_THR = '1';
    *UART3_THR = '2';
    *UART3_THR = '3';
    *UART3_THR = '\n';
    while (1) { }
}
