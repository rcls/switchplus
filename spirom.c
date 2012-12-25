#include "monkey.h"
#include "registers.h"
#include "spirom.h"

#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)
#define STATIC_ASSERT(b) extern int JOIN(_sa_dummy_, __LINE__)[b ? 1 : -1]

void spirom_init(void)
{
    // 8 bits.
    // Format = spi.
    // cpol - clock high between frames.
    // cpha - capture data on second clock transition.
    // divide clock by 2*(63+1) = 128.
    // Not sure what the story is with clocking...
    *BASE_SSP1_CLK = 0x0e000800;        // Base clock is 96MHz.
    *CLK_M4_SSP1_CFG = 1;               // Enable register clock.
    *CLK_APB2_SSP1_CFG = 1;             // Enable peripheral clock.

    /* printf ("Addresses %p %p %p %p %p %p %p %p\n", */
    /*         BASE_SSP1_CLK, CLK_M4_SSP1_CFG, CLK_APB2_SSP1_CFG, */
    /*         &SSP1->cr0, &SSP1->cr1, &SSP1->dr, &SSP1->sr, &SSP1->cpsr); */

    printf ("Reset SSP1");

    RESET_CTRL[1] = 1 << 19;            // Reset ssp1.
    while (!(RESET_ACTIVE_STATUS[1] & (1 << 19)));

    for (int i = 0; i != 100000; ++i)
        asm volatile ("");

    SSP1->cpsr = 20;
    SSP1->cr0 = 0x3f07;
    SSP1->cr1 = 2;                      // Enable master.

    // Setup pins; make CS a GPIO output.
    GPIO_BYTE[7][19] = 1;
    GPIO_DIR[7] |= 1 << 19;

    SFSP[15][4] = 0;                    // SCK is D10, PF_4 func 0.
    SFSP[15][5] = 4;                    // SSEL is E9, PF_5, GPIO7[19] func 4.
    SFSP[15][6] = 0x42;                 // MISO is E7, PF_6 func 2.
    SFSP[15][7] = 2;                    // MOSI is B7, PF_7 func 2.

    printf ("\nGet id:");

    // Wait for idle.
    while (SSP1->sr & 16);

    // Take CS low.
    GPIO_BYTE[7][19] = 0;

    SSP1->dr = 0x9f;
    for (int i = 0; i != 6; ++i)
        SSP1->dr = 0;

    for (int i = 0; i != 7; ++i) {
        int j = 0;
        while (!(SSP1->sr & 4))
            if (++j >= 1000000) {
                printf (" -");
                break;
            }
        printf (" %02x", SSP1->dr);
    }

    printf ("\n");

    // Wait for idle.
    while (SSP1->sr & 16);

    // Take CS high.
    GPIO_BYTE[7][19] = 0;
}
