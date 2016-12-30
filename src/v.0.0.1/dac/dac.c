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
	Last Update:	16.11.2011
	Last Update:	15.01.2012
	Full Change:	02.02.2012 - For Board v.0.0.5 (10.01.2012)
	Last Update:	02.02.2012
*/

#include "dac.h"

extern uint16_t uiRegHolding[];

void dacLoadDefConst(uint16_t *dacConst)
{
	// Калибриращи константи на DAC. Представляват зададенета стойност
	// на канал разделена на изходното напрежение, и накрая умножено
	// по десет (за запазване на първия разряд след дес.точка):
	// DACnConst = 10 * (63535 / XX.XX V):
	dacConst[0] = 63896;
	dacConst[1] = 63896;
}

void lpcDacSet(unsigned char ch, unsigned int data, unsigned int d)
{
	float a = (float)data / (float)d, c=a;

	switch(ch) {
	case 0: c = uiRegHolding[12] / 100.0;
		//while(false == DAC_Channel_DataEmpty(&DACA, CH0));
		//DAC_Channel_Write(&DACA, (unsigned int)(c * (float)a), CH0);
	 break;
	case 1: c = uiRegHolding[13] / 100.0;
		//while(false == DAC_Channel_DataEmpty(&DACA, CH1));
		//DAC_Channel_Write(&DACA, (unsigned int)(c * (float)a), CH1);
	 break;
	}
}

void initDac(void)
{
	select_analog_out();
	unselect_analog_out();
}

void writeDac(uint16_t dac_a, uint16_t dac_b)
{
	//dac_a <<= 4;
	//dac_b <<= 4;
	//dac_a &= 0xfff0;
	//dac_b &= 0xfff0;

	select_analog_out();
	spi_send_byte(0x30);
	spi_send_byte(dac_a>>8);
	spi_send_byte(dac_a<<4);
	unselect_analog_out();

	select_analog_out();
	spi_send_byte(0x31);
	spi_send_byte(dac_b>>8);
	spi_send_byte(dac_b<<4);
	unselect_analog_out();
}
