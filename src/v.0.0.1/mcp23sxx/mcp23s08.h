#ifndef __MCP23S08_H__
#define __MCP23S08_H__

#define MCP23S08_IODIR		0x00
#define MCP23S08_IPOL		0x01
#define MCP23S08_GPINTE		0x02
#define MCP23S08_DEFVAL		0x03
#define MCP23S08_INTCON		0x04
#define MCP23S08_IOCON		0x05
#define MCP23S08_GPPU		0x06
#define MCP23S08_INTF		0x07
#define MCP23S08_INTCAP		0x08
#define MCP23S08_GPIO		0x09
#define MCP23S08_OLAT		0x0A

void mcp23s08_ddr_init(char ddr);
void mcp23s08_input_polarity(char ipol);

#endif
