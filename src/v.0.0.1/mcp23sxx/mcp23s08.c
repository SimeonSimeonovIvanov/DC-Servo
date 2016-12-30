#include "mcp23sxx.h"
#include "mcp23s08.h"

/*
	If bit 0 - Digital input
	If bit 1 - Digital output
*/
void mcp23s08_ddr_init(char ddr)
{
	mcp23sxx_spi_write(MCP23S08_IODIR, ~ddr);
}

/*
	If Bit 0 - Read actual value
	If Bit 1 - Read invertet value
*/
void mcp23s08_input_polarity(char ipol)
{
	mcp23sxx_spi_write(MCP23S08_IPOL, ipol);
}
