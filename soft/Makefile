BUILDS=flashA flashB flashAhi flashBhi

all: ${BUILDS:%=main.%.bin} flash.sram.bin dfu.sram.bin

.PHONY: all clean
.SECONDARY:

clean:
	-rm -f *.elf *.bin *.flasher *.boot *.o *.a *.s .deps/*.d

CC=arm-none-eabi-gcc
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none -fwhole-program
OBJCOPY=arm-none-eabi-objcopy
CFLAGS=-Os -flto -ffreestanding \
	-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -MMD -MP -MF.deps/$@.d \
	-fno-common -Wall -Werror
UTILS=../utils

-include .deps/*.d

LIBO=callback configure discus freq jtag monitor monkey sdram spirom switch usb

$(BUILDS:%=main.%.elf) main.sram.elf: $(LIBO:%=%.o)

# Kill this rule.
%: %.c

%.sram.elf: sram.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@

%.flashA.elf: flashA.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@
%.flashB.elf: flashB.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@
%.flashAhi.elf: flashAhi.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@
%.flashBhi.elf: flashBhi.ld %.o
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.boot: %.bin
	$(UTILS)/addheader < $< > $@

%.dfu: %
	cp $< $@.tmp
	dfu-suffix -v 0xf055 -p 0x524c -a $@.tmp
	mv $@.tmp $@

%.flasher: %.bin flash.sram.bin
	$(UTILS)/flasher flash.sram.bin $< > $@.tmp
	$(UTILS)/addheader < $@.tmp > $@.tmp2
	rm $@.tmp
	mv $@.tmp2 $@

$(BUILDS:%=%.ld): %.ld: template.ld.in
	sed 's/@TARGET@/$*/g' $< > $@.tmp
	mv $@.tmp $@

.PHONY: go
# We don't really depend on the flasher.  But let's make it harder to
# accidentally program an older version into the flash.
go:	main.sram.boot main.flashA.flasher $(BUILDS:%=main.%.bin)
	dfu-util -D main.sram.boot

%.s: %.c
	$(CC) $(filter-out -flto,$(CFLAGS)) -S -o $@ $<

# Cancel...
%: %.c
%: %.o
