
CFLAGS=-O2 -g -Wall -Werror -std=gnu11 -flto -fwhole-program -MMD -MP -MF.deps/$@.d

LINK.o = $(CC) $(CFLAGS) $(LDFLAGS)

PROGS=addheader decode_bitstream flasher runit xbit pbit usbreset square4 \
	flashmon
all: $(PROGS)

clean:
	rm -f *.o $(PROGS) .deps/*.d

$(PROGS): util.o

-include .deps/*.d

# Cancel...
%: %.c
