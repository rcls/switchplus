
.PHONY: all clean
all: main.sram.boot main.flashA.flasher dfu.sram.bin

.SECONDARY:

clean:
	-rm -f *.elf *.bin *.flasher *.boot *.o *.a *.s .deps/*.d

CC=arm-linux-gnu-gcc
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none -fwhole-program
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -flto -std=gnu11 -ffreestanding \
	-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -MMD -MP -MF.deps/$@.d \
	-fno-common -Wall -Werror
UTILS=../utils

-include .deps/*.d

LIBO=callback configure freq jtag lcd monkey sdram spirom square switch usb
main.sram.elf main.flashA.elf: $(LIBO:%=%.o)

# Kill this rule.
%: %.c

^-ld = $(filter-out %.ld,$^)

%.sram.elf: sram.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@

%.flashA.elf: flashA.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.boot: %.bin
	$(UTILS)/addheader < $< > $@

%.dfu: %
	cp $< $@.tmp
	/home/ralph/dfu-util/src/dfu-suffix -v 0xf055 -p 0x4c52 -a $@.tmp
	mv $@.tmp $@

%.flasher: %.bin flash.sram.bin
	$(UTILS)/flasher flash.sram.bin $< > $@.tmp
	$(UTILS)/addheader < $@.tmp > $@.tmp2
	rm $@.tmp
	mv $@.tmp2 $@

%.ld: template.ld.in
	sed 's/@TARGET@/$*/g' $< > $@.tmp
	mv $@.tmp $@

.PHONY: go
# We don't really depend on the flasher.  But let's make it harder to
# accidentally program an older version into the flash.
go:	main.sram.boot main.flashA.flasher
	dfu-util -D main.sram.boot

%.s: %.c
	$(CC) $(filter-out -flto,$(CFLAGS)) -S -o $@ $<

# Cancel...
%: %.c
%: %.o
