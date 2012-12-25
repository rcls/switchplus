
CFLAGS=-O2 -Wall -Werror -std=gnu11 -flto -fwhole-program -MMD -MP -MF.deps/$@.d

PROGS=addheader flasher runit xbit pbit decode_bitstream
all: $(PROGS)

clean:
	rm -f *.o $(PROGS) .deps/*.d

$(PROGS): util.o

-include .deps/*.d
