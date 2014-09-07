
.PHONY: all clean
all: main.sram.boot main.flashA.flasher dfu.sram.bin

.SECONDARY:

clean:
	-rm -f *.elf *.bin *.flasher *.boot *.o *.a *.s .deps/*.d

CC=arm-linux-gnu-gcc
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -flto -fwhole-program -std=gnu11 -ffreestanding \
	-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -MMD -MP -MF.deps/$@.d \
	-fno-common -fdata-sections -Wall -Werror
UTILS=../utils

-include .deps/*.d

LIBO=callback pin freq jtag lcd monkey sdram spirom square switch usb
main.sram.elf main.flashA.elf: $(LIBO:%=%.o)

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
	$(UTILS)/addheader < $< > $@

%.dfu: %
	cp $< $@.tmp
	/home/ralph/dfu-util/src/dfu-suffix -v 0xf055 -p 0x4c52 -a $@.tmp
	mv $@.tmp $@

%.flasher: %.bin flash.zero.bin
	$(UTILS)/flasher flash.zero.bin $< > $@.tmp
	$(UTILS)/addheader < $@.tmp > $@.tmp2
	rm $@.tmp
	mv $@.tmp2 $@

.PHONY: go
# We don't really depend on the flasher.  But let's make it harder to
# accidentally program an older version into the flash.
go:	main.sram.boot main.flashA.flasher
	sudo dfu-util -D main.sram.boot
