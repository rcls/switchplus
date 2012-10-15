#ifndef SWITCH_H_
#define SWITCH_H

// Send 1..4 bytes.  Data is big endian, lsb aligned.
unsigned spi_io (unsigned data, int num_bytes);

unsigned spi_reg_read (unsigned address);
unsigned spi_reg_write (unsigned address, unsigned data);

#endif
