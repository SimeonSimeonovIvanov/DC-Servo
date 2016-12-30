#ifndef __MCP23S17_H__
#define __MCP23S17_H__

#define IOCON_BANCK0

#ifdef IOCON_BANCK0
	#define MCP23S17_IODIRA		0x00
	#define MCP23S17_IODIRB		0x01
	#define MCP23S17_IPOLA		0x02
	#define MCP23S17_IPOLB		0x03
	#define MCP23S17_GPINTEA	0x04
	#define MCP23S17_GPINTEB	0x05
	#define MCP23S17_DEFVALA	0x06
	#define MCP23S17_DEFVALB	0x07
	#define MCP23S17_INTCONA	0x08
	#define MCP23S17_INTCONB	0x09
	#define MCP23S17_IOCON		0x0A
	//#define MCP23S17_IOCON	0x0B - Same as address 0x0A
	#define MCP23S17_GPPUA		0x0C
	#define MCP23S17_GPPUB		0x0D
	#define MCP23S17_INTFA		0x0E
	#define MCP23S17_INTFB		0x0F
	#define MCP23S17_INTCAPA	0x10
	#define MCP23S17_INTCAPB	0x11
	#define MCP23S17_GPIOA		0x12
	#define MCP23S17_GPIOB		0x13
	#define MCP23S17_OLATA		0x14
	#define MCP23S17_OLATB		0x15
#endif

#ifdef IOCON_BANCK1
	#define MCP23S17_IODIRA		0x00
	#define MCP23S17_IPOLA		0x01
	#define MCP23S17_GPINTENA	0x02
	#define MCP23S17_DEFVALA	0x03
	#define MCP23S17_INTCONA	0x04
	#define MCP23S17_IOCON		0x05
	#define MCP23S17_GPPUA		0x06
	#define MCP23S17_INTFA		0x07
	#define MCP23S17_INTCAPA	0x08
	#define MCP23S17_GPIOA		0x09
	#define MCP23S17_OLATA		0x0A

	#define MCP23S17_IODIRB		0x10
	#define MCP23S17_IPOLB		0x11
	#define MCP23S17_GPINTENB	0x12
	#define MCP23S17_DEFVALB	0x13
	#define MCP23S17_INTCONB	0x14
	//#define MCP23S17_IOCON	0x15
	#define MCP23S17_GPPUB		0x16
	#define MCP23S17_INTFB		0x17
	#define MCP23S17_INTCAPB	0x18
	#define MCP23S17_GPIOB		0x19
	#define MCP23S17_OLATB		0x1A
#endif

///////////////////////////////////////////////////////////
/*
	If bit 0 - Digital input
	If bit 1 - Digital output
*/
void mcp23s17_ddra_init(char ddra);
void mcp23s17_ddrb_init(char ddrb);
///////////////////////////////////////////////////////////
/*
	If Bit 0 - Read actual value
	If Bit 1 - Read invertet value
*/
void mcp23s17_input_polarity_a(char ipola);
void mcp23s17_input_polarity_b(char ipolb);
///////////////////////////////////////////////////////////
void mcp23s17_write_outa(char outa);
void mcp23s17_write_outb(char outb);
////////////////////////////////////////////////////////////

#endif
