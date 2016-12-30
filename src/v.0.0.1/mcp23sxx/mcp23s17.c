#include "mcp23sxx.h"
#include "mcp23s17.h"

///////////////////////////////////////////////////////////
/*
	If bit 0 - Digital input
	If bit 1 - Digital output
*/
void mcp23s17_ddra_init(char ddra)
{
	mcp23sxx_spi_write(MCP23S17_IODIRA, ~ddra);
}

void mcp23s17_ddrb_init(char ddrb)
{
	mcp23sxx_spi_write(MCP23S17_IODIRB, ~ddrb);
}
///////////////////////////////////////////////////////////
/*
	If Bit 0 - Read actual value
	If Bit 1 - Read invertet value
*/
void mcp23s17_input_polarity_a(char ipola)
{
	mcp23sxx_spi_write(MCP23S17_IPOLA, ipola);
}

void mcp23s17_input_polarity_b(char ipolb)
{
	mcp23sxx_spi_write(MCP23S17_IPOLB, ipolb);
}
///////////////////////////////////////////////////////////
void mcp23s17_write_outa(char outa)
{
	mcp23sxx_spi_write(MCP23S17_GPIOA, outa);
}

void mcp23s17_write_outb(char outb)
{
	mcp23sxx_spi_write(MCP23S17_GPIOB, outb);
}
////////////////////////////////////////////////////////////
