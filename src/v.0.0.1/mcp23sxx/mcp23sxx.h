#ifndef __MCP23SXX_H__
#define __MCP23SXX_H__

#define mcp23sxx_spi_set()		spi_high_frequency()

char mcp23sxx_spi_read(char address);
void mcp23sxx_spi_write(char address, char data);

#endif
