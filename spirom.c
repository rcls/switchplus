#include "monkey.h"
#include "registers.h"
#include "spirom.h"

#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)
#define STATIC_ASSERT(b) extern int JOIN(_sa_dummy_, __LINE__)[b ? 1 : -1]
#define CS1 (&GPIO_BYTE[7][19])
#define CLR "\r\e[K"

#define PAGE_LEN 264

static bool buffer_select;

static void spi_start (volatile ssp_t * ssp, volatile unsigned char * cs,
                       unsigned op)
{
    // Wait for idle & clear out the fifo..
    while (ssp->sr & 20)
        ssp->dr;

    *cs = 0;
    ssp->dr = op;
}


static void spi_end (volatile ssp_t * ssp, volatile unsigned char * cs)
{
    while (ssp->sr & 16);              // Wait for idle.
    *cs = 1;
}


static void op_address (volatile ssp_t * ssp, volatile unsigned char * cs,
                        unsigned op, unsigned address)
{
    spi_start(ssp, cs, op);
    ssp->dr = address >> 16;
    ssp->dr = address >> 8;
    ssp->dr = address;
}


static bool spirom_idle(volatile ssp_t * ssp, volatile unsigned char * cs)
{
    spi_start(ssp, cs, 0xd7);
    while (!(ssp->sr & 4));
    ssp->dr;
    for (int i = 0; i != 1048576; ++i) {
        ssp->dr = 0;
        while (!(ssp->sr & 4));
        if (ssp->dr & 0x80) {
            spi_end(ssp, cs);
            return true;
        }
    }
    spi_end(ssp, cs);
    return false;
}


static void write_page (volatile ssp_t * ssp, volatile unsigned char * cs,
                        unsigned page, const unsigned char * data)
{
    unsigned buffer_write = buffer_select ? 0x84 : 0x87;

    op_address(ssp, cs, buffer_write, 0);

    for (int i = 0; i != PAGE_LEN; ++i) {
        while (!(ssp->sr & 2));
        ssp->dr = data[i];
    }
    spi_end(ssp, cs);

    // Wait for idle.
    if (!spirom_idle(ssp, cs)) {
        printf("*** Idle wait failed for page 0x%0x\n", page);
        return;
    }

    unsigned page_write = buffer_select ? 0x83 : 0x86;
    buffer_select = !buffer_select;

    op_address(ssp, cs, page_write, page * 512);
    spi_end(ssp, cs);
}


void spirom_init(void)
{
    // 8 bits.
    // Format = spi.
    // cpol - clock high between frames.
    // cpha - capture data on second clock transition.
    // divide clock by 2*(63+1) = 128.
    *BASE_SSP1_CLK = 0x0e000800;        // Base clock is 96MHz.

    printf("Reset SSP1");

    RESET_CTRL[1] = (1 << 19) | (1 << 24); // Reset ssp1, keep m0 in reset.
    while (!(RESET_ACTIVE_STATUS[1] & (1 << 19)));

    SSP1->cpsr = 20;
    SSP1->cr0 = 0x3f07;
    SSP1->cr1 = 2;                      // Enable master.

    // Setup pins; make CS a GPIO output.
    *CS1 = 1;
    GPIO_DIR[7] |= 1 << 19;

    SFSP[15][4] = 0;                    // SCK is D10, PF_4 func 0.
    SFSP[15][5] = 4;                    // SSEL is E9, PF_5, GPIO7[19] func 4.
    SFSP[15][6] = 0x42;                 // MISO is E7, PF_6 func 2.
    SFSP[15][7] = 2;                    // MOSI is B7, PF_7 func 2.

    printf("\nGet id:");

    // Wait for idle.
    spi_start(SSP1, CS1, 0x9f);
    for (int i = 0; i != 6; ++i)
        SSP1->dr = 0;

    for (int i = 0; i != 7; ++i) {
        while (!(SSP1->sr & 4));
        printf(" %02x", SSP1->dr);
    }

    printf("\n");

    spi_end(SSP1, CS1);

    spi_start(SSP1, CS1, 0xd7);
    SSP1->dr = 0;
    SSP1->dr = 0;
    spi_end (SSP1, CS1);
    SSP1->dr;
    printf("Status = %02x %02x\n", SSP1->dr, SSP1->dr);
}


static int hex_nibble (int c)
{
    if (c >= '0' && c <= '9') {
        if (verbose_flag)
            putchar(c);
        return c - '0';
    }
    c &= ~32;
    if (c >= 'A' && c <= 'F') {
        if (verbose_flag)
            putchar(c);
        return c - 'A' + 10;
    }
    printf(CLR "Illegal hex character; aborting...");
    if (c != '\n')
        while (getchar() != '\n');
    putchar('\n');
    return -1;
}


static void spirom_program(void)
{
    verbose(CLR "SPIROM program page: ");
    unsigned page = 0;
    while (true) {
        int c = getchar();
        if (c == ':')
            break;
        int n = hex_nibble(c);
        if (n < 0)
            return;
        page = page * 16 + n;
    }
    verbose(CLR "SPIROM program page 0x%x: ", page);
    unsigned char data[PAGE_LEN];
    for (int i = 0; i != PAGE_LEN; ++i) {
        int n1 = hex_nibble(getchar());
        if (n1 < 0)
            return;
        int n2 = hex_nibble(getchar());
        if (n2 < 0)
            return;
        data[i] = n1 * 16 + n2;
    }
    verbose(CLR "SPIROM program page 0x%x press ENTER: ", page);
    if (getchar() == '\n')
        write_page (SSP1, CS1, page, data);
    else
        printf ("Aborted\n");
}


static void spirom_read(void)
{
    verbose(CLR "SPIROM read page: ");
    unsigned page = 0;
    while (true) {
        int c = getchar();
        if (c == '\n')
            break;
        int n = hex_nibble(c);
        if (n < 0)
            return;
        page = page * 16 + n;
    }
    printf(CLR "SPIROM read page 0x%x\n", page);
    op_address(SSP1, CS1, 0xb, page * 512);
    int i = 0;
    int j = 0;
    while (j < PAGE_LEN + 5 || (SSP1->sr & 0x15) != 1) {
        if (i < PAGE_LEN + 1 && (SSP1->sr & 2)) {
            ++i;
            SSP1->dr = 0;
        }
        if (SSP1->sr & 4) {
            if (j++ < 5)
                SSP1->dr;
            else
                printf("%02x%s", SSP1->dr, (j & 15) == 5 ? "\n" : " ");
        }
    }
    spi_end(SSP1, CS1);
    printf("\n");
}


void spirom_command(void)
{
    spirom_init();

    while (true) {
        verbose(CLR "SPIROM: <r>ead, <p>rogram, <i>dle, <s>tatus...");
        switch (getchar()) {
        case 'p':
            spirom_program();
            break;
        case 'r':
            spirom_read();
            break;
        case 's':
            spi_start(SSP1, CS1, 0xd7);
            SSP1->dr = 0;
            spi_end(SSP1, CS1);
            SSP1->dr;
            printf(CLR "Status = %02x\n", SSP1->dr);
            break;
        case 'i':
            verbose(CLR "Idle wait");
            if (!spirom_idle(SSP1, CS1))
                printf(CLR "Idle wait failed!\n");
            else
                verbose(CLR "SPIROM idle\n");
            break;
        default:
            printf(CLR "SPIROM exit...\n");
            return;
        }
    }
}
