
CFLAGS=-O2 -Wall -Werror -std=gnu11 -flto -fwhole-program -MMD -MP -MF.deps/$@.d

LINK.o = $(CC) $(CFLAGS) $(LDFLAGS)

PROGS=addheader flasher runit xbit pbit decode_bitstream usbreset
all: $(PROGS)

clean:
	rm -f *.o $(PROGS) .deps/*.d

$(PROGS): util.o

-include .deps/*.d

# Cancel...
%: %.c
