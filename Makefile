
.PHONY: all clean
all: main.sram.boot main.flashA.flasher dfu.sram.bin

.SECONDARY:

clean:
	-rm *.elf *.bin *.flasher *.boot *.o *.a *.s

CC=arm-linux-gnu-gcc
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -flto -fwhole-program -std=gnu99 -ffreestanding \
	-mcpu=cortex-m4 -mthumb -MMD -MP -MF.$@.d \
	-fno-common -fdata-sections  -Wall -Werror -Wno-error=unused-function

-include .*.d

main.sram.elf main.flashA.elf: liblpc.a

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<

# Kill this rule.
%: %.c

%.sram.elf: %.o
	$(LINK.c) -T sram.ld $^ $(LOADLIBES) $(LDLIBS) -o $@

%.zero.elf: %.o
	$(LINK.c) -T zero.ld $^ $(LOADLIBES) $(LDLIBS) -o $@

%.flashA.elf: %.o
	$(LINK.c) -T flashA.ld $^ $(LOADLIBES) $(LDLIBS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.boot: %.bin
	utils/addheader < $< > $@

%.dfu: %
	cp $< $@.tmp
	/home/ralph/dfu-util/src/dfu-suffix -v 0xf055 -p 0x4c52 -a $@.tmp
	mv $@.tmp $@

%.flasher: %.bin flash.zero.bin
	utils/flasher flash.zero.bin $< > $@.tmp
	utils/addheader < $@.tmp > $@.tmp2
	rm $@.tmp
	mv $@.tmp2 $@

liblpc.a: freq.o monkey.o sdram.o switch.o usb.o
	ar cr $@ $+
