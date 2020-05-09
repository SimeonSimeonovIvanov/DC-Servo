/*
		simeon_s._ivanov@abv.bg

	Create Date:	07.10.2010
	Last Update:	10.10.2010

	Full Change:	XX.07.2011 - For Board v.0.0.5
	Last Update:	20.08.2011
	Last Update:	20.08.2011 - Add DAC Support.
	Last Update:	29.08.2011 - Ana Now 'Run' RS485 Suport...
	Last Update:	30.08.2011
	Last Update:	01.09.2011
	Last Update:	04.09.2011 - Добавяне опция за настройка на UART по ModBUS
	Last Update:	05.09.2011 - Добаване ModBUS Time Out

			This File:
	Create Date:	07.11.2011
	Last Update:	16.11.2011
	Last Update:	16.08.2015 - For Board v.0.0.8
	Last Update:	23.08.2015
*/

#include "adc.h"
#include <string.h>
extern uint16_t arrADC[7];

//////////////////////////////////////////////////////////////////////////////////////
void adcLoadDefConst(uint16_t *adcConst)
{
	// Калибриращи константи на ADC. Представляват измерените
	// стоиности на канал при фиксирано входно напрежение (10.00 V):
	adcConst[0] = 40950;		adcConst[1] = 40950;
	adcConst[2] = 40950;		adcConst[3] = 40950;
	adcConst[4] = 40950;		adcConst[5] = 40950;
	adcConst[6] = 40950;
}

void initAdc(void)
{
	select_analog_in();
	unselect_analog_in();
}

void readADC(void)
{
	uint16_t i, arrTemp[7];
	unsigned char index[7] = {
		1, 5, 2, 6,
		3, // ???
		0, 4
	};

	for( i = 0; i < 7; i++ ) {	
		arrTemp[i] = readAdcChannel(i);
	}

	for( i = 0; i < 7; i++ ) {
		arrADC[i] = arrTemp[ index[i] ];
	}
}

uint16_t readAdcChannel( uint8_t n )
{
	volatile uint16_t msb, lsb;

	uint8_t cmd[7] = {
		0b10001100,	// CH0 CH7/COM
		0b10011100,	// CH2 CH7/COM
		0b10101100,	// CH4 CH7/COM
		0b10111100,	// CH6 CH7/COM
		0b11001100,	// CH1 CH7/COM
		0b11011100,	// CH3 CH7/COM
		0b11101100	// CH5 CH7/COM
	};
	
	select_analog_in();
	SPCR |= (1<<SPR0);

	SPDR = cmd[n];
	while( !( SPSR & (1 << SPIF) ) );
	msb = SPDR;

	SPDR = 0;
	while( !( SPSR & (1 << SPIF) ) );
	lsb = SPDR;

	SPCR &= ~(1<<SPR0);
	unselect_analog_in();
	
	return ( (0x0ff0 & (msb<<4)) | (0x000f & (lsb>>4)) );
}
