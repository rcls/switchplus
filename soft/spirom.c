#include "callback.h"
#include "configure.h"
#include "monkey.h"
#include "registers.h"
#include "spirom.h"

#include <stddef.h>

#define ROM_CS (&GPIO_BYTE[3][8])
#define CLR "\r\e[K"

#define PAGE_LEN 264

static bool buffer_select;

static void spi_start(unsigned op)
{
    monkey_ssp_off();

    SSP1->cr0 = 0x4f07;                  // divide-by-80 (1MHz), 8 bits.

    *ROM_CS = 0;
    SSP1->dr = op;
}


static void spi_end(void)
{
    while (SSP1->sr & 16);               // Wait for idle.
    *ROM_CS = 1;

    SSP1->cr0 = 0x0007;                  // divide-by-1.

    monkey_ssp_on();
}


static void op_address(unsigned op, unsigned address)
{
    spi_start(op);
    SSP1->dr = address >> 16;
    SSP1->dr = address >> 8;
    SSP1->dr = address;
}


static bool spirom_idle(void)
{
    spi_start(0xd7);
    while (!(SSP1->sr & 4));
    SSP1->dr;
    for (int i = 0; i != 1048576; ++i) {
        SSP1->dr = 0;
        while (!(SSP1->sr & 4));
        if (SSP1->dr & 0x80) {
            spi_end();
            return true;
        }
    }
    spi_end();
    return false;
}


static void write_page(unsigned page, const unsigned char * data)
{
    unsigned buffer_write = buffer_select ? 0x84 : 0x87;

    op_address(buffer_write, 0);

    for (int i = 0; i != PAGE_LEN; ++i) {
        while (!(SSP1->sr & 2));
        SSP1->dr = data[i];
    }
    spi_end();

    // Wait for idle.
    if (!spirom_idle()) {
        printf("*** Idle wait failed for page 0x%0x\n", page);
        return;
    }

    unsigned page_write = buffer_select ? 0x83 : 0x86;
    buffer_select = !buffer_select;

    op_address(page_write, page * 512);
    spi_end();
}


static void spirom_init(void)
{
    // 8 bits.
    // Format = spi.
    // cpol - clock high between frames.
    // cpha - capture data on second clock transition.
    // We rely on the ssp log having set up the SSP.

    // Set up B16, P7_0 as function 0, GPIO3_8.  High slew rate.
    SFSP[7][0] = 0x20;
    BIT_BAND(GPIO_DIR[3])[8] = 1;

    printf("\nGet id:");

    // Wait for idle.
    spi_start(0x9f);
    for (int i = 0; i != 6; ++i)
        SSP1->dr = 0;

    unsigned char bytes[7];
    for (int i = 0; i != 7; ++i) {
        while (!(SSP1->sr & 4));
        bytes[i] = SSP1->dr;
    }

    spi_end();

    for (int i = 0; i != 7; ++i)
        printf(" %02x", bytes[i]);
    printf("\n");

    spi_start(0xd7);
    SSP1->dr = 0;
    SSP1->dr = 0;
    while (SSP1->sr & 16);              // Wait for idle.
    SSP1->dr;
    int s = SSP1->dr;
    int t = SSP1->dr;
    spi_end();
    printf("Status = %02x %02x\n", s, t);
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
    restart_program(NULL);
}


static unsigned get_page(const char * tag, unsigned terminator)
{
    verbose(CLR "SPIROM %s page: ", tag);
    unsigned page = 0;
    while (true) {
        int c = getchar();
        if (c == terminator)
            return page;
        page = page * 16 + hex_nibble(c);
    }
}


static void spirom_program(void)
{
    unsigned page = get_page("program", ':');
    verbose(CLR "SPIROM program page 0x%x: ", page);
    unsigned char data[PAGE_LEN];
    for (int i = 0; i != PAGE_LEN; ++i) {
        int n1 = hex_nibble(getchar());
        int n2 = hex_nibble(getchar());
        data[i] = n1 * 16 + n2;
    }
    verbose(CLR "SPIROM program page 0x%x press ENTER: ", page);
    if (getchar() == '\n')
        write_page(page, data);
    else
        printf ("Aborted\n");
}


static void spirom_read(void)
{
    unsigned page = get_page("read", '\n');
    printf(CLR "SPIROM read page 0x%x\n", page);
    unsigned char bytes[PAGE_LEN + 5];
    op_address(0xb, page * 512);
    int i = 0;
    int j = 0;
    while (j < PAGE_LEN + 5 || (SSP1->sr & 0x15) != 1) {
        if (i < PAGE_LEN + 1 && (SSP1->sr & 2)) {
            ++i;
            SSP1->dr = 0;
        }
        if (SSP1->sr & 4)
            bytes[j++] = SSP1->dr;
    }
    spi_end();
    for (int i = 5; i < PAGE_LEN + 5; ++i)
        printf("%02x%s", bytes[i], (i & 15) == 4 ? "\n" : " ");
    if (PAGE_LEN & 15)
        printf("\n");
}


static void _Noreturn spirom_run(void)
{
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
            spi_start(0xd7);
            SSP1->dr = 0;
            spi_end();
            SSP1->dr;
            printf(CLR "Status = %02x\n", SSP1->dr);
            break;
        case 'i':
            verbose(CLR "Idle wait");
            if (!spirom_idle())
                printf(CLR "Idle wait failed!\n");
            else
                verbose(CLR "SPIROM idle\n");
            break;
        default:
            printf(CLR "SPIROM exit...\n");
            start_program(initial_program);
        }
    }
}


void spirom_command(void)
{
    spirom_init();
    start_program(spirom_run);
}
