
all: main.sram.boot main.flashA.flasher

CC=arm-linux-gnu-gcc
LD=arm-linux-gnu-ld
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -Wall -Werror -std=gnu99 -mcpu=cortex-m4 -mthumb \
	-ffreestanding -fno-common -flto -fwhole-program \
	-ffunction-sections -fdata-sections -Wno-error=unused-function \
	-MMD -MP -MF.$@.d

-include .*.d

main.sram.elf main.flashA.elf: liblpc.a

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<

%: %.c

%.sram.elf: %.o
	$(LINK.c) -T sram.ld $^ $(LOADLIBES) $(LDLIBS) -o $@

%.zero.elf: %.o
	$(LINK.c) -T zero.ld $^ $(LOADLIBES) $(LDLIBS) -o $@

%.flashA.elf: %.o
	$(LINK.c) -T flashA.ld $^ $(LOADLIBES) $(LDLIBS) -o $@

%: %.elf
	$(OBJCOPY) -O binary $< $@

%.boot: %
	utils/addheader < $< > $@

%.dfu: %
	cp $< $@.tmp
	/home/ralph/dfu-util/src/dfu-suffix -v 0x1fc9 -p 0x000c -a $@.tmp
	mv $@.tmp $@

flash.zero:

%.flasher: % flash.zero
	utils/flasher flash.zero $< > $@.tmp
	utils/addheader < $@.tmp > $@.tmp2
	rm $@.tmp
	mv $@.tmp2 $@

liblpc.a: freq.o monkey.o switch.o usb.o
	ar cr $@ $+
