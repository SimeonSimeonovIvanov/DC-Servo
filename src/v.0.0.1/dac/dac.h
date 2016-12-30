/*
		The^day^of^DooM

	Create Date:	07.10.2010
	Last Update:	10.10.2010

	Full Change:	XX.07.2011 - For Board v.0.0.5 (17.01.2011)
	Last Update:	20.08.2011
	Last Update:	20.08.2011 - Add DAC Support.
	Last Update:	29.08.2011 - Ana Now 'Run' RS485 Suport...
	Last Update:	30.08.2011
	Last Update:	01.09.2011
	Last Update:	04.09.2011 - Добавяне опция за настройка на UART по ModBUS
	Last Update:	05.09.2011 - Добаване ModBUS Time Out

			This File:
	Create Date:	07.11.2011
	Last Update:	07.11.2011
	Last Update:	15.01.2012
	Full Change:	02.02.2012 - For Board v.0.0.5 (10.01.2012)
	Last Update:	02.02.2012
*/

#include <stdio.h>
#include "../main.h"

#define NUMBER_OF_DAC_CHANNEL			2

void initDac(void);
void dacLoadDefConst(uint16_t *dacConst);
void lpcDacSet(unsigned char ch, unsigned int data, unsigned int d);

void writeDac(uint16_t dac_a, uint16_t dac_b);
