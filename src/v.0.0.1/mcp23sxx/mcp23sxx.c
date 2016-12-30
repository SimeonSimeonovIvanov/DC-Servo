#include <avr/io.h>
#include "mcp23sxx.h"

char mcp23sxx_spi_read(char address)
{
	SPDR = 0x41;
	while(!(SPSR & (1<<SPIF)));

	SPDR = address;
	while(!(SPSR & (1<<SPIF)));

	SPDR = 0;
	while(!(SPSR & (1<<SPIF)));

	return SPDR;
}

void mcp23sxx_spi_write(char address, char data)
{
	SPDR = 0x40;
	while(!(SPSR & (1<<SPIF)));

	SPDR = address;
	while(!(SPSR & (1<<SPIF)));

	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
}
