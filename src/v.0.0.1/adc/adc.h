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
	Last Update:	08.11.2011
*/

#include <stdio.h>
#include "../main.h"

#define __ADC_READ_FUNC11__

#define __ADC_OFFSET__					0
#define __ADC_BITS_MASK__ 				0xffff//e
#define __ADC_NUMBER_OF_SAMPLES__		8

#define NUMBER_OF_ADC_CHANNEL			7

void initAdc(void);
void readADC(void);
uint16_t readAdcChannel( uint8_t n );
void adcLoadDefConst(uint16_t *adcConst);
/////////////////////////////////////////////////////
