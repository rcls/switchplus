
all: blink.bin

RENAME=mv $*-gerber/$*.$1 $*-gerber/$($1_ext)
DELETE=rm $*-gerber/$*.$1

sw-gerbers: top.gbr_ext=TopSide.gbr
sw-gerbers: topmask.gbr_ext=TopSolderMask.gbr
sw-gerbers: topsilk.gbr_ext=TopSilkscreen.gbr
sw-gerbers: bottom.gbr_ext=BotSide.gbr
sw-gerbers: bottommask.gbr_ext=BotSoldermask.gbr
sw-gerbers: bottomsilk.gbr_ext=BotSilkscreen.gbr
sw-gerbers: outline.gbr_ext=BoardOutline.gbr
sw-gerbers: plated-drill.cnc_ext=Drill.cnc
sw-gerbers: group1.gbr_ext=Innerlayer1.gbr
sw-gerbers: group2.gbr_ext=Innerlayer2.gbr

.PHONY: %-gerbers
%-gerbers:
	mkdir -p $*-gerber
	cp $*.pcb $*-gerber/
	cd $*-gerber && /home/archive/bin/pcb -x gerber --metric $*.pcb --outfile foo
	$(call RENAME,top.gbr)
	$(call RENAME,topmask.gbr)
	$(call DELETE,toppaste.gbr)
	$(call RENAME,topsilk.gbr)
	$(call RENAME,bottom.gbr)
	$(call RENAME,bottommask.gbr)
	$(call DELETE,bottompaste.gbr)
	-$(call RENAME,bottomsilk.gbr)
	-$(call RENAME,group1.gbr)
	-$(call RENAME,group2.gbr)
	$(call RENAME,outline.gbr)
	$(call DELETE,fab.gbr)
	$(call RENAME,plated-drill.cnc)
	rm $*-gerber/$*.pcb

%.zip: %-gerbers
	-rm $*.zip
	cd $*-gerber && zip ../$*.zip *.{txt,gbr,cnc}

.PHONY: zips
zips: sw.zip


W=www
sw-png: LS=Top,TopPwr,GND,GNDmsc,Power,PSig,BotPwr,Bot

PCBPNG=/home/archive/bin/pcb -x png --as-shown --use-alpha --layer-stack silk,$(LS)

PNGS=sw-png
.PHONY: pngs $(PNGS)
$(PNGS): %-png: $W/%.png $W/%-s.png $W/%b.png $W/%b-s.png

pngs: $(PNGS)

$W/%.png: %.pcb
	$(PCBPNG) --outfile $@ --dpi 300 $<
$W/%-s.png: %.pcb
	$(PCBPNG) --outfile $@ --dpi 100 $<
$W/%b.png: %.pcb
	$(PCBPNG),solderside --outfile $@ --dpi 300 $<
$W/%b-s.png: %.pcb
	$(PCBPNG),solderside --outfile $@ --dpi 100 $<

CC=arm-linux-gnu-gcc
LD=arm-linux-gnu-ld
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -Wall -Werror -std=gnu99 -march=armv7-m -mthumb -ffreestanding -Wno-error=unused-function

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<

%: %.c

#	$(LD) --Ttext=0x10000000 -r -o $@ $+
%.bin.elf: %.o
	$(LD) -T sram-link.ld -o $@ $+

%.bin: %.bin.elf
	$(OBJCOPY) -O binary $< $@

%.zero.elf: %.o
	$(LD) -T zero.ld -o $@ $+

%.zero: %.zero.elf
	$(OBJCOPY) -O binary $< $@

%.boot: %
	utils/addheader < $< > $@

%.dfu: %
	cp $< $@.tmp
	/home/ralph/dfu-util/src/dfu-suffix -v 0x1fc9 -p 0x000c -a $@.tmp
	mv $@.tmp $@
