#include "callback.h"
#include "freq.h"
#include "monkey.h"
#include "registers.h"

#include <stddef.h>

#define CLR "\r\e[K"

typedef struct command_t {
    const char * name;
    void (*func)(const unsigned *, int);
    unsigned short min;
    unsigned short max;
} command_t;

bool locked;


// Return number of components (not including terminator) or -1 on error.
// May take two more than max.
int getsplitline(char * line, int max)
{
    int comps = 0;
    bool inword = false;
    for (int i = 0; i < max;) {
        int c = getchar();
        if (c > ' ') {
            line[i++] = c;
            inword = true;
            if (verbose_flag)
                putchar(c);
            continue;
        }
        if (c == '\e')
            restart_program("\n");

        if (inword) {
            inword = false;
            ++comps;
            line[i++] = 0;
        }
        if (c == '\n') {
            line[i++] = 0;
            if (verbose_flag)
                putchar(c);
            return comps;
        }
        if (verbose_flag)
            putchar(' ');
    }
    drop_line_restart(CLR "Line too long...", 0);
}


static unsigned hextou(const char * h)
{
    unsigned result = 0;
    do {
        unsigned c = *h;
        unsigned v = c - '0';
        if (c >= 'A')
            v = (c & ~32) - 'A' + 10;
        if (v > 15)
            restart_program("Illegal hex\n");

        result = result * 16 + v;
    }
    while (*++h);
    return result;
}


static bool streq(const char * a, const char * b)
{
    for (; *a == *b; ++a, ++b)
        if (!*a)
            return true;
    return false;
}


static const char * strnext(const char * s)
{
    while (*s++);
    return s;
}


static unsigned checksum(const volatile unsigned char * p, unsigned count)
{
    unsigned csum = ~0;
    for (int i = 0; i < count; ++i)
        csum = csum * 253 + p[i];
    return csum;
}


static void flash_command(const unsigned * in, unsigned * out,
                          bool check)
{
    unsigned command = in[0];

    typedef void IAP_t(const unsigned *, unsigned *);
    IAP_t * iap = * (IAP_t **) 0x10400100;
    __interrupt_disable();
    iap(in, out);
    __interrupt_enable();
    if (!check || out[0] == 0)
        return;

    printf("flash IAP %u fails: %i\n", command, out[0]);
    restart_program("");
}


static unsigned flash_bank(unsigned address)
{
    return (address & 0x01000000) == 0 ? 0 : 1;
}


static unsigned flash_sector(unsigned address)
{
    // The first 64kB is split into 8x8k sectors, then 7 of 64kB.
    unsigned by_8k = (address >> 13) & 0xff;
    if (by_8k < 8)
        return by_8k;

    return (by_8k >> 3) + 7;
}


static unsigned sane_cpu_freq(void)
{
    unsigned freq = cpu_frequency(1000);
    if (freq >= 95000 || freq <= 161000)
        return freq;

    printf("write: bogus CPU frequency %u kHz.\n", freq);
    restart_program("");
}


static void command_checksum(const unsigned * restrict args, int comps)
{
    unsigned csum = checksum((unsigned char *) args[0], args[1]);
    if (comps == 3 && csum != args[2]) {
        printf("checksum error got %08x expect %08x\n", csum, args[2]);
        restart_program("");
    }
    if (comps != 3 || verbose_flag)
        printf("checksum %08x\n", csum);
}


static void command_word(const unsigned * args, int comps)
{
    if (comps == 3)
        * (volatile unsigned *) args[0] = args[1];
    else
        printf("%08x: %08x\n", args[0], * (volatile unsigned *) args[0]);
}


static void command_read(const unsigned * args, int comps)
{
    const unsigned char * p = (const unsigned char *) args[0];
    const unsigned char * end = p + args[1];
    while (p != end) {
        int c = *p++;
        char sep = ' ';
        if (((unsigned) p & 15) == 0 || p == end)
            sep = '\n';
        printf("%02x%c", c, sep);
    }
}


static void command_go(const unsigned * args, int comps)
{
    unsigned * vector = (unsigned *) *args;
    __interrupt_disable();
    CREG100->m4memmap = vector;
    asm volatile("mov sp,%0\n" "bx %1" :: "r"(vector[0]), "r"(vector[1]));
}


static void command_call(const unsigned * args, int comps)
{
    typedef unsigned func_t(unsigned, unsigned, unsigned, unsigned);
    func_t * f = (func_t *) args[0];
    unsigned r = f(args[1], args[2], args[3], args[4]);
    printf("function returns %08x\n", r);
}


static void command_unlock(const unsigned * restrict args, int comps)
{
    unsigned arg = '1';
    flash_command(&arg, &arg, false);
    locked = false;
    puts("unlocked\n");
}


static void flash_bank_sector(unsigned command,
                              unsigned base, unsigned length,
                              unsigned freq)
{
    if ((base & 0xff000000) == ((unsigned) command_go & 0xff000000))
        restart_program("flash bank is in use.\n");

    unsigned p[5];
    p[0] = command;
    p[1] = flash_sector(base);
    p[2] = flash_sector(base + length);
    unsigned bank = flash_bank(base);
    if (freq != 0) {
        p[3] = freq;
        p[4] = bank;
    }
    else
        p[3] = flash_bank(base);
    flash_command(p, p, true);
}


static void command_erase(const unsigned * restrict args, int comps)
{
    unsigned base = args[0];
    unsigned length = args[1];

    // FIXME - check sector alignment.  FIXME - check length.
    if (length == 0)
        return;

    unsigned freq = sane_cpu_freq();
    flash_bank_sector('2', base, length, 0);    // Prepare.
    flash_bank_sector('4', base, length, freq); // Erase.

    puts("erase: ok\n");
}


static void command_flash(const unsigned * restrict args, int comps)
{
    unsigned base = args[0];
    unsigned length = args[1];
    unsigned csum = args[2];
    // Should fit within the 512kB block at 0x1a000000 or 0x1b000000,
    // and have correct alignment and length.
    if ((length != 512 && length != 1024 && length != 4096)
        || (base & 511) != 0
        || (base & ~0x01000000) < 0x1a000000
        || (base & ~0x01000000) + length > 0x01a080000)
        restart_program("flash: invalid arguments\n");

    puts("flash: send data.\n");
    unsigned char data[length];
    get_hex_block(data, length);

    if (locked)
        restart_program("flash: locked.\n");

    // Blank check.
    for (unsigned i = 0; i < length; ++i)
        if (* (signed char *) (base + i) != -1)
            restart_program("flash: not erased.\n");

    if (csum != checksum(data, length))
        restart_program("flash: checksum mismatch.\n");

    unsigned freq = sane_cpu_freq();

    flash_bank_sector('2', base, length, 0); // Prepare.

    unsigned p[] = { '3', base, (unsigned) data, length, freq };
    flash_command(p, p, true);          // Write flash.

    // Verify.
    for (unsigned i = 0; i < length; ++i)
        if (* (unsigned char *) (base + i) != data[i])
            restart_program("flash: verify failed.\n");

    puts("flash: success\n");
}


static void command_write(const unsigned * restrict args, int comps)
{
    puts("write: send data.\n");
    get_hex_block((unsigned char *) args[0], args[1]);
    puts("write: ok.\n");
}


static const command_t commands[] = {
    { "go", command_go, 1, 1 },
    { "word", command_word, 1, 2 },
    { "read", command_read, 2, 2 },
    { "call", command_call, 1, 5 },
    { "write", command_write, 2, 2 },
    { "checksum", command_checksum, 2, 3 },
    { "erase", command_erase, 2, 2 },
    { "flash", command_flash, 3, 3 },
    { "unlock", command_unlock, 0, 0 },
    { NULL, NULL, 0, 0 }
};

void run_command(const char * line, int comps)
{
    for (const command_t * p = commands; p->name; ++p) {
        if (!streq(p->name, line))
            continue;

        --comps;
        if (comps < p->min || comps > p->max) {
            printf("%s: takes between %u and %u parameters\n",
                   line, p->min, p->max);
            locked = true;
            return;
        }

        unsigned args[5];
        const char * a = line;
        for (int i = 0; i < comps; ++i) {
            a = strnext(a);
            args[i] = hextou(a);
        }
        p->func(args, comps);
        return;
    }

    puts("Unknown command\n");
}


static void _Noreturn monitor(void)
{
    locked = true;
    char line[82];
    while (1) {
        verbose("MON> ");
        int comps = getsplitline(line, 80);
        if (comps <= 0) {
            verbose("Exit monitor\n");
            start_program(initial_program);
        }
        run_command(line, comps);
    }
}


void run_monitor(void)
{
    start_program(monitor);
}
