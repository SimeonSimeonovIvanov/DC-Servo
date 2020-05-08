/******************************************
 * Title		: Microchip ENCX24J600 Ethernet Interface Driver
 * Author		: Jiri Melnikov
 * Created		: 28.12.2009
 * Version		: 0.1
 * Target MCU	: Atmel AVR series
 *
 * Description	: This driver provides initialization and transmit/receive
 *				functions for the Microchip ENCX24J600 100Mb Ethernet
 *				Controller and PHY. Only supported interface is SPI, no
 *				PSP interface available by now. No security functions are
 *				are supported by now.
 *
 *				This driver is inspired by ENC28J60 driver from Pascal
 *				Stang (2005).
 *
 *				Many lines of code are taken from Microchip's TCP/IP stack.
 * 
 * ****************************************/

#include <util/delay.h>

#include <avr/sfr_defs.h>

#include "enc424j600.h"
#include "../../arch/spi.h"

// Binary constant identifiers for ReadMemoryWindow() and WriteMemoryWindow()
// functions
#define UDA_WINDOW				(0x1)
#define GP_WINDOW				(0x2)
#define RX_WINDOW				(0x4)

// Promiscuous mode, uncomment if you want to receive all packets, even those which are not for you
//#define PROMISCUOUS_MODE

// Internal MAC level variables and flags.
static uint8_t currentBank;
static uint16_t nextPacketPointer;

static uint16_t enc424j600ReadReg(uint16_t address);
static void enc424j600WriteReg(uint16_t address, uint16_t data);
static void enc424j600ExecuteOp0(uint8_t op);
static void enc424j600BFSReg(uint16_t address, uint16_t bitMask);
static void enc424j600BFCReg(uint16_t address, uint16_t bitMask);
static void enc424j600WriteN(uint8_t op, uint8_t* data, uint16_t dataLen);
static void enc424j600ReadN(uint8_t op, uint8_t* data, uint16_t dataLen);

/********************************************************************
 * INITIALIZATION
 * ******************************************************************/
void enc424j600Init(uint8_t *mac_addr)
{
	//Set default bank
	currentBank = 0;
	unselect_net_chip();

	//sbi(ENC424J600_SPI_PORT, ENC424J600_SPI_SCK);
	//sbi(ENC424J600_SPI_DDR, ENC424J600_SPI_SCK);

	//sbi(ENC424J600_SPI_DDR, ENC424J600_SPI_MOSI);
	//cbi(ENC424J600_SPI_DDR, ENC424J600_SPI_MISO);

	enc424j600_spi_set();

	// Perform a reliable reset
	enc424j600SendSystemReset();

	// Initialize RX tracking variables and other control state flags
	nextPacketPointer = RXSTART;

	// Set up TX/RX/UDA buffer addresses
	enc424j600WriteReg(ETXST, TXSTART);
	enc424j600WriteReg(ERXST, RXSTART);
	enc424j600WriteReg(ERXTAIL, RAMSIZE - 2);
	enc424j600WriteReg(EUDAST, RAMSIZE);
	enc424j600WriteReg(EUDAND, RAMSIZE + 1);

	// Get MAC adress
	uint16_t regValue;
	regValue = enc424j600ReadReg(MAADR1);
	mac_addr[0] = ((uint8_t*) & regValue)[0];
	mac_addr[1] = ((uint8_t*) & regValue)[1];
	regValue = enc424j600ReadReg(MAADR2);
	mac_addr[2] = ((uint8_t*) & regValue)[0];
	mac_addr[3] = ((uint8_t*) & regValue)[1];
	regValue = enc424j600ReadReg(MAADR3);
	mac_addr[4] = ((uint8_t*) & regValue)[0];
	mac_addr[5] = ((uint8_t*) & regValue)[1];

	// If promiscuous mode is set, than allow accept all packets
	#ifdef PROMISCUOUS_MODE
	enc424j600WriteReg(ERXFCON,(ERXFCON_CRCEN | ERXFCON_RUNTEN | ERXFCON_UCEN | ERXFCON_NOTMEEN | ERXFCON_MCEN));
	#endif

	// Set PHY Auto-negotiation to support 10BaseT Half duplex,
	// 10BaseT Full duplex, 100BaseTX Half Duplex, 100BaseTX Full Duplex,
	// and symmetric PAUSE capability
	enc424j600WritePHYReg(PHANA, PHANA_ADPAUS0 | PHANA_AD10FD | PHANA_AD10 | PHANA_AD100FD | PHANA_AD100 | PHANA_ADIEEE0);

	// Enable RX packet reception
	enc424j600BFSReg(ECON1, ECON1_RXEN);
}

/********************************************************************
 * UTILS
 * ******************************************************************/
void enc424j600SendSystemReset(void)
{
	// Perform a reset via the SPI/PSP interface
	do {
		// Set and clear a few bits that clears themselves upon reset.
		// If EUDAST cannot be written to and your code gets stuck in this
		// loop, you have a hardware problem of some sort (SPI or PMP not
		// initialized correctly, I/O pins aren't connected or are
		// shorted to something, power isn't available, etc.)
		//sbi(PORTE, PE7);
		do {
			enc424j600WriteReg(EUDAST, 0x1234);
		} while (enc424j600ReadReg(EUDAST) != 0x1234);
		// Issue a reset and wait for it to complete
		enc424j600BFSReg(ECON2, ECON2_ETHRST);
		currentBank = 0;
		while( (enc424j600ReadReg(ESTAT) & (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY)) != (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY) );
		_delay_us(300);
		// Check to see if the reset operation was successful by
		// checking if EUDAST went back to its reset default.  This test
		// should always pass, but certain special conditions might make
		// this test fail, such as a PSP pin shorted to logic high.
	} while (enc424j600ReadReg(EUDAST) != 0x0000u);

	// Really ensure reset is done and give some time for power to be stable
	_delay_us(1000);
}

/**
 * Is link connected?
 * @return <char>
 */
char enc424j600MACIsLinked(void)
{
	return (enc424j600ReadReg(ESTAT) & ESTAT_PHYLNK) != 0u;
}

/**
 * Is transmission active?
 * @return <char>
 */
char enc424j600MACIsTxReady(void)
{
	return !(enc424j600ReadReg(ECON1) & ECON1_TXRTS);
}

char enc424j600PacketSend(uint8_t* packet, uint16_t len)
{
	spi_high_frequency();

	// Set the Window Write Pointer to the beginning of the transmit buffer
	enc424j600WriteReg(EGPWRPT, TXSTART);

#ifdef HARDWARE_CHECKSUM_NULL
	// Is it the IP packet? If so, for sure null checksum a let hardware to compute it
	if (packet[12] == IP_PROTOCOL1 && packet[13] == IP_PROTOCOL2) {
		//clear IP checksum
		packet[24] = 0;
		packet[25] = 0;
		//we can also compute icmp/tcp/udp messages
		if (packet[IP_PROTOCOL_POS] == ICMP_PROTOCOL) {
			//clear ICMP checksum
			packet[36] = 0;
			packet[37] = 0;
		} else if (packet[IP_PROTOCOL_POS] == TCP_PROTOCOL) {
			//clear TCP checksum
			packet[50] = 0;
			packet[51] = 0;
		} else if (packet[IP_PROTOCOL_POS] == UDP_PROTOCOL) {
			//clear UDP checksum
			packet[40] = 0;
			packet[41] = 0;
		}
	}
#endif

	enc424j600WriteMemoryWindow(GP_WINDOW, packet, len);

#ifdef HARDWARE_CHECKSUM
	// Is it the IP packet? Get it computed by hardware
	if (packet[12] == IP_PROTOCOL1 && packet[13] == IP_PROTOCOL2) {
		//Compute header length
		uint8_t headerLen = (packet[ETH_HEADER] & 15)*4;
		//Compute checksum of IP header
		uint16_t checksum = enc424j600ChecksumCalculation(ETH_HEADER, headerLen, 0x0000);
		//Write it to correct position
		enc424j600WriteReg(EGPWRPT, 24);
		enc424j600WriteMemoryWindow(GP_WINDOW, ((uint8_t*) & checksum), 2);

		//we can also compute icmp/tcp/udp messages
		if (packet[IP_PROTOCOL_POS] == ICMP_PROTOCOL) { /*ICMP*/
			//Compute header length
			uint8_t icmpLen = len - headerLen - ETH_HEADER;
			//Compute checksum of ICMP
			checksum = enc424j600ChecksumCalculation(ETH_HEADER + headerLen, icmpLen, 0x0000);
			//Write it to correct position
			enc424j600WriteReg(EGPWRPT, 36);
			enc424j600WriteMemoryWindow(GP_WINDOW, ((uint8_t*) & checksum), 2);
		} else if (packet[IP_PROTOCOL_POS] == TCP_PROTOCOL || packet[IP_PROTOCOL_POS] == UDP_PROTOCOL) { /*TCP or UDP*/
			//Compute header length
			uint16_t upperLayerLen = len - headerLen - ETH_HEADER;

			//Compute checksum of TCP or UDP
			checksum = ~(HTONS(packet[IP_PROTOCOL_POS] + upperLayerLen)); //HTONS macro is from uIP
			checksum = enc424j600ChecksumCalculation(26, 8, checksum);
			checksum = enc424j600ChecksumCalculation(ETH_HEADER + headerLen, upperLayerLen, checksum);

			//Write it to correct position
			if (packet[IP_PROTOCOL_POS] == TCP_PROTOCOL) {
				enc424j600WriteReg(EGPWRPT, 50);
			} else {
				enc424j600WriteReg(EGPWRPT, 40);
			}
			enc424j600WriteMemoryWindow(GP_WINDOW, ((uint8_t*) & checksum), 2);
		}
	}
#endif

	enc424j600WriteReg(ETXLEN, len);

	enc424j600MACFlush();

	return 1;
}

uint16_t enc424j600PacketReceive(uint8_t* packet, uint16_t len)
{
	uint16_t newRXTail;
	RXSTATUS statusVector;

	spi_high_frequency();

	if(!(enc424j600ReadReg(EIR) & EIR_PKTIF)) {
		return 0;
	}

#ifdef AUTO_ICMP_ECHO  //Check if packet is ICMP echo packet and answer to it automaticaly
	//Set buffer for packet data
	uint8_t packetData[2];
	// Set the RX Read Pointer to the beginning of the next unprocessed packet + statusVektor + nextPacketPointer + position where paket type is saved
	enc424j600WriteReg(ERXRDPT, nextPacketPointer + sizeof (statusVector) + ETH_HEADER);

	//Read type of paket first, if it's IP
	enc424j600ReadMemoryWindow(RX_WINDOW, packetData, sizeof (packetData));
	if (packetData[0] == IP_PROTOCOL1 && packetData[1] == IP_PROTOCOL2) {
		//Ok, it's ip packet, check if it's icmp packet
		enc424j600WriteReg(ERXRDPT, nextPacketPointer + 2 + sizeof (statusVector) + IP_PROTOCOL_POS);
		enc424j600ReadMemoryWindow(RX_WINDOW, packetData, 1);
		if (packetData[0] == ICMP_PROTOCOL) {
			//It's icmp packet, read lenght and do DMA copy operation from recieve buffer to transmit buffer
			enc424j600WriteReg(ERXRDPT, nextPacketPointer + 2);
			enc424j600ReadMemoryWindow(RX_WINDOW, packetData, 2);
			if (*(uint16_t*) packetData < 1522) {
				//Now do DMA copy, first read length from IP packet
				uint16_t ipPacketLen;
				uint8_t ipHeaderLen;
				enc424j600WriteReg(ERXRDPT, nextPacketPointer + 2 + sizeof (statusVector) + 14);
				enc424j600ReadMemoryWindow(RX_WINDOW, (uint8_t*) & ipHeaderLen, 1);
				ipHeaderLen = (ipHeaderLen & 15)*4;
				enc424j600WriteReg(ERXRDPT, nextPacketPointer + 2 + sizeof (statusVector) + 16);
				enc424j600ReadMemoryWindow(RX_WINDOW, (uint8_t*) & ipPacketLen, 2);
				ipPacketLen = HTONS(ipPacketLen);
				//Wait until controler is ready
				while (enc424j600ReadReg(ECON1) & ECON1_DMAST) {
				}
				//Set DMA copy and no checksum while copying (checksum computing at the end will be faster)
				//Switch MAC addr
				enc424j600BFSReg(ECON1, ECON1_DMACPY);
				enc424j600BFSReg(ECON1, ECON1_DMANOCS);
				enc424j600WriteReg(EDMAST, nextPacketPointer + 2 + sizeof (statusVector) + 6); //Switch MAC addr in packet
				enc424j600WriteReg(EDMADST, TXSTART);
				enc424j600WriteReg(EDMALEN, 6);
				enc424j600BFSReg(ECON1, ECON1_DMAST); // Wait until done
				while (enc424j600ReadReg(ECON1) & ECON1_DMAST) {
				}
				enc424j600WriteReg(EDMAST, nextPacketPointer + 2 + sizeof (statusVector)); //Switch MAC addr in packet
				enc424j600WriteReg(EDMADST, TXSTART + 6);
				enc424j600WriteReg(EDMALEN, 6);
				enc424j600BFSReg(ECON1, ECON1_DMAST); // Wait until done
				while (enc424j600ReadReg(ECON1) & ECON1_DMAST) {
				}
				enc424j600WriteReg(EDMAST, nextPacketPointer + 2 + sizeof (statusVector) + 12); //Copy packet
				enc424j600WriteReg(EDMADST, TXSTART + 12);
				enc424j600WriteReg(EDMALEN, ipPacketLen + 2);
				enc424j600BFSReg(ECON1, ECON1_DMAST); // Wait until done
				while (enc424j600ReadReg(ECON1) & ECON1_DMAST) {
				}
				//Switch IP addr
				enc424j600WriteReg(EDMAST, nextPacketPointer + 2 + sizeof (statusVector) + 26); //Switch IP addr in packet
				enc424j600WriteReg(EDMADST, TXSTART + 30);
				enc424j600WriteReg(EDMALEN, 4);
				enc424j600BFSReg(ECON1, ECON1_DMAST); // Wait until done
				while (enc424j600ReadReg(ECON1) & ECON1_DMAST) {
				}
				enc424j600WriteReg(EDMAST, nextPacketPointer + 2 + sizeof (statusVector) + 30); //Switch IP addr in packet
				enc424j600WriteReg(EDMADST, TXSTART + 26);
				enc424j600WriteReg(EDMALEN, 4);
				enc424j600BFSReg(ECON1, ECON1_DMAST); // Wait until done
				while (enc424j600ReadReg(ECON1) & ECON1_DMAST) {
				}
				//Change echo request to echo reply
				packetData[0] = 0;
				packetData[1] = 0;
				enc424j600WriteReg(EGPWRPT, 34);
				enc424j600WriteMemoryWindow(GP_WINDOW, packetData, 1);
				enc424j600WriteReg(EGPWRPT, 36);
				enc424j600WriteMemoryWindow(GP_WINDOW, packetData, 2);
				//Compute checksum (use packetData for mem saving)
				*(uint16_t*) packetData = enc424j600ChecksumCalculation(ETH_HEADER + ipHeaderLen, ipPacketLen - ipHeaderLen, 0x0000);
				//Write it to the packet
				enc424j600WriteReg(EGPWRPT, 36);
				enc424j600WriteMemoryWindow(GP_WINDOW, packetData, 2);
				//Flush packet out
				enc424j600WriteReg(ETXLEN, ipPacketLen + ETH_HEADER);
				enc424j600MACFlush();
			}
			enc424j600WriteReg(ERXRDPT, nextPacketPointer);
			enc424j600ReadMemoryWindow(RX_WINDOW, (uint8_t*) & nextPacketPointer, sizeof (nextPacketPointer));
			newRXTail = nextPacketPointer - 2;
			//Special situation if nextPacketPointer is exactly RXSTART
			if (nextPacketPointer == RXSTART)
				newRXTail = RAMSIZE - 2;
			//Packet decrement
			enc424j600BFSReg(ECON1, ECON1_PKTDEC);
			//Write new RX tail
			enc424j600WriteReg(ERXTAIL, newRXTail);
			//
			return 0;
		}
	} 
#endif

	// Set the RX Read Pointer to the beginning of the next unprocessed packet
	enc424j600WriteReg(ERXRDPT, nextPacketPointer);

	enc424j600ReadMemoryWindow(RX_WINDOW, (uint8_t*) & nextPacketPointer, sizeof (nextPacketPointer));

	enc424j600ReadMemoryWindow(RX_WINDOW, (uint8_t*) & statusVector, sizeof (statusVector));
	len = (statusVector.bits.ByteCount <= len+4) ? statusVector.bits.ByteCount-4 : 0;
	enc424j600ReadMemoryWindow(RX_WINDOW, packet, len);

	newRXTail = nextPacketPointer - 2;
	//Special situation if nextPacketPointer is exactly RXSTART
	if (nextPacketPointer == RXSTART)
		newRXTail = RAMSIZE - 2;

	//Packet decrement
	enc424j600BFSReg(ECON1, ECON1_PKTDEC);

	//Write new RX tail
	enc424j600WriteReg(ERXTAIL, newRXTail);

	return len;
}

void enc424j600MACFlush(void)
{
	uint16_t w;

	// Check to see if the duplex status has changed.  This can
	// change if the user unplugs the cable and plugs it into a
	// different node.  Auto-negotiation will automatically set
	// the duplex in the PHY, but we must also update the MAC
	// inter-packet gap timing and duplex state to match.
	if(enc424j600ReadReg(EIR) & EIR_LINKIF) {
		enc424j600BFCReg(EIR, EIR_LINKIF);

		// Update MAC duplex settings to match PHY duplex setting
		w = enc424j600ReadReg(MACON2);
		if (enc424j600ReadReg(ESTAT) & ESTAT_PHYDPX) {
			// Switching to full duplex
			enc424j600WriteReg(MABBIPG, 0x15);
			w |= MACON2_FULDPX;
		} else {
			// Switching to half duplex
			enc424j600WriteReg(MABBIPG, 0x12);
			w &= ~MACON2_FULDPX;
		}
		enc424j600WriteReg(MACON2, w);
	}


	// Start the transmission, but only if we are linked.  Supressing
	// transmissing when unlinked is necessary to avoid stalling the TX engine
	// if we are in PHY energy detect power down mode and no link is present.
	// A stalled TX engine won't do any harm in itself, but will cause the
	// MACIsTXReady() function to continuously return 0, which will
	// ultimately stall the Microchip TCP/IP stack since there is blocking code
	// elsewhere in other files that expect the TX engine to always self-free
	// itself very quickly.
	if (enc424j600ReadReg(ESTAT) & ESTAT_PHYLNK)
		enc424j600BFSReg(ECON1, ECON1_TXRTS);
}

/********************************************************************
 * READERS AND WRITERS
 * ******************************************************************/

void enc424j600WriteMemoryWindow(uint8_t window, uint8_t *data, uint16_t length)
{
	uint8_t op = RBMUDA;

	if (window & GP_WINDOW)
		op = WBMGP;
	if (window & RX_WINDOW)
		op = WBMRX;

	enc424j600WriteN(op, data, length);
}

void enc424j600ReadMemoryWindow(uint8_t window, uint8_t *data, uint16_t length)
{
	if (length == 0u)
		return;

	uint8_t op = RBMUDA;

	if (window & GP_WINDOW)
		op = RBMGP;
	if (window & RX_WINDOW)
		op = RBMRX;

	enc424j600ReadN(op, data, length);
}

/**
 * Reads from address
 * @variable <uint16_t> address - register address
 * @return <uint16_t> data - data in register
 */
static uint16_t enc424j600ReadReg(uint16_t address)
{
	uint16_t returnValue;
	uint8_t bank;

	// See if we need to change register banks
	bank = ((uint8_t) address) & 0xE0;
	if (bank <= (0x3u << 5)) {
		if (bank != currentBank) {
			if (bank == (0x0u << 5))
				enc424j600ExecuteOp0(B0SEL);
			else if (bank == (0x1u << 5))
				enc424j600ExecuteOp0(B1SEL);
			else if (bank == (0x2u << 5))
				enc424j600ExecuteOp0(B2SEL);
			else if (bank == (0x3u << 5))
				enc424j600ExecuteOp0(B3SEL);

			currentBank = bank;
		}
		returnValue = enc424j600ExecuteOp16(RCR | (address & 0x1F), 0x0000);
	} else {
		uint32_t returnValue32 = enc424j600ExecuteOp32(RCRU, (uint32_t) address);
		((uint8_t*) & returnValue)[0] = ((uint8_t*) & returnValue32)[1];
		((uint8_t*) & returnValue)[1] = ((uint8_t*) & returnValue32)[2];
	}

	return returnValue;
}

/**
 * Writes to register
 * @variable <uint16_t> address - register address
 * @variable <uint16_t> data - data to register
 */
static void enc424j600WriteReg(uint16_t address, uint16_t data)
{
	uint8_t bank;

	// See if we need to change register banks
	bank = ((uint8_t) address) & 0xE0;
	if (bank <= (0x3u << 5)) {
		if (bank != currentBank) {
			if (bank == (0x0u << 5))
				enc424j600ExecuteOp0(B0SEL);
			else if (bank == (0x1u << 5))
				enc424j600ExecuteOp0(B1SEL);
			else if (bank == (0x2u << 5))
				enc424j600ExecuteOp0(B2SEL);
			else if (bank == (0x3u << 5))
				enc424j600ExecuteOp0(B3SEL);

			currentBank = bank;
		}
		enc424j600ExecuteOp16(WCR | (address & 0x1F), data);
	} else {
		uint32_t data32;
		((uint8_t*) & data32)[0] = (uint8_t) address;
		((uint8_t*) & data32)[1] = ((uint8_t*) & data)[0];
		((uint8_t*) & data32)[2] = ((uint8_t*) & data)[1];
		enc424j600ExecuteOp32(WCRU, data32);
	}

}

uint16_t enc424j600ReadPHYReg(uint8_t address)
{
	uint16_t returnValue;

	// Set the right address and start the register read operation
	enc424j600WriteReg(MIREGADR, 0x0100 | address);
	enc424j600WriteReg(MICMD, MICMD_MIIRD);

	// Loop to wait until the PHY register has been read through the MII
	// This requires 25.6us
	while (enc424j600ReadReg(MISTAT) & MISTAT_BUSY);

	// Stop reading
	enc424j600WriteReg(MICMD, 0x0000);

	// Obtain results and return
	returnValue = enc424j600ReadReg(MIRD);

	return returnValue;
}

void enc424j600WritePHYReg(uint8_t address, uint16_t Data)
{
	// Write the register address
	enc424j600WriteReg(MIREGADR, 0x0100 | address);

	// Write the data
	enc424j600WriteReg(MIWR, Data);

	// Wait until the PHY register has been written
	while (enc424j600ReadReg(MISTAT) & MISTAT_BUSY);
}

static void enc424j600ReadN(uint8_t op, uint8_t* data, uint16_t dataLen)
{
	select_net_chip();

	// issue read command
	SPDR = op;
	// wail until all is sent
	while (!(SPSR & (1 << SPIF)));

	while (dataLen--) {
		// wait for answer
		SPDR = 0x00;
		while(!(SPSR & (1 << SPIF)));
		*data++ = SPDR;
	}

	unselect_net_chip();
}

static void enc424j600WriteN(uint8_t op, uint8_t* data, uint16_t dataLen)
{
	select_net_chip();

	// issue read command
	SPDR = op;
	// wail until all is sent
	while(!(SPSR & (1 << SPIF)));
	
	while(dataLen--) {
		// start sending data to SPI
		SPDR = *data++;
		// wail until all is sent
		while (!(SPSR & (1 << SPIF)));
	}
	
	unselect_net_chip();
}

static void enc424j600BFSReg(uint16_t address, uint16_t bitMask)
{
	uint8_t bank;

	// See if we need to change register banks
	bank = ((int8_t) address) & 0xE0;
	if (bank != currentBank) {
		if (bank == (0x0u << 5))
			enc424j600ExecuteOp0(B0SEL);
		else if (bank == (0x1u << 5))
			enc424j600ExecuteOp0(B1SEL);
		else if (bank == (0x2u << 5))
			enc424j600ExecuteOp0(B2SEL);
		else if (bank == (0x3u << 5))
			enc424j600ExecuteOp0(B3SEL);

		currentBank = bank;
	}

	enc424j600ExecuteOp16(BFS | (address & 0x1F), bitMask);
}

static void enc424j600BFCReg(uint16_t address, uint16_t bitMask)
{
	uint8_t bank;

	// See if we need to change register banks
	bank = ((uint8_t) address) & 0xE0;
	if (bank != currentBank) {
		if (bank == (0x0u << 5))
			enc424j600ExecuteOp0(B0SEL);
		else if (bank == (0x1u << 5))
			enc424j600ExecuteOp0(B1SEL);
		else if (bank == (0x2u << 5))
			enc424j600ExecuteOp0(B2SEL);
		else if (bank == (0x3u << 5))
			enc424j600ExecuteOp0(B3SEL);

		currentBank = bank;
	}

	enc424j600ExecuteOp16(BFC | (address & 0x1F), bitMask);
}
/********************************************************************
 * EXECUTES
 * ******************************************************************/

/**
 * Execute SPI operation
 * @variable <uint8_t> op - operation
 */
static void enc424j600ExecuteOp0(uint8_t op)
{
	uint8_t dummy;

	select_net_chip();

	// issue read command
	SPDR = op;
	// wail until all is sent
	while (!(SPSR & (1 << SPIF)));
	// read answer
	dummy = SPDR;

	unselect_net_chip();
}

/**
 * Write data to SPI with operation
 * @variable <uint8_t> op - SPI operation
 * @variable <uint8_t> data - data
 */
uint8_t enc424j600ExecuteOp8(uint8_t op, uint8_t data)
{
	uint8_t returnValue;

	select_net_chip();

	// issue write command
	SPDR = op;
	// wail until all is sent
	while (!(SPSR & (1 << SPIF)));
	// start sending data to SPI
	SPDR = data;
	// wain until all is sent
	while (!(SPSR & (1 << SPIF)));
	// read answer
	returnValue = SPDR;

	unselect_net_chip();

	return returnValue;
}

/**
 * Write data to SPI with operation
 * @variable <uint8_t> op - SPI operation
 * @variable <uint16_t> data - data
 */
uint16_t enc424j600ExecuteOp16(uint8_t op, uint16_t data)
{
	uint16_t x, returnValue;

	select_net_chip();

	// issue write command
	SPDR = op;
	// wail until all is sent
	while (!(SPSR & (1 << SPIF)));
	// in this cycle, data are sent
	for
	(x = 0; x < 2; x++) {
		// start sending data to SPI
		SPDR = ((uint8_t*) & data)[x];
		// wain until all is sent
		while (!(SPSR & (1 << SPIF)));
		// read answer
		((uint8_t*) & returnValue)[x] = SPDR;
	}

	unselect_net_chip();

	return returnValue;
}

/**
 * Write data to SPI with operation
 * @variable <uint8_t> op - SPI operation
 * @variable <uint32_t> data - data
 */
uint32_t enc424j600ExecuteOp32(uint8_t op, uint32_t data)
{
	uint32_t returnValue;

	select_net_chip();

	// issue write command
	SPDR = op;
	// wail until all is sent
	while (!(SPSR & (1 << SPIF)));
	// in this cycle, data are sent
	for (int x = 0; x < 3; x++) {
		// start sending data to SPI
		SPDR = ((uint8_t*) & data)[x];
		// wain until all is sent
		while (!(SPSR & (1 << SPIF)));
		// read answer
		((uint8_t*) & returnValue)[x] = SPDR;
	}

	unselect_net_chip();

	return returnValue;
}
