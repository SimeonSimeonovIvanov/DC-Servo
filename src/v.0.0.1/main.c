/**
 * \mainpage mega-eth - Atmel AVR ethernet board
 *
 * This project is a basis for developing microcontroller applications
 * connected to an ethernet network.
 *
 * \section hardware The hardware
 *
 * Hardware components already integrated on the reference design include:
 * - Atmel ATmega128 RISC microcontroller with standard 10-pin ISP header
 * - 64 kByte of external SRAM
 * - USB \<-\> RS232 interface
 * - SD/MMC socket
 * - Ethernet interface with ENC28J60 (IEEE 802.3, 10Base-T)
 *
 * The hardware design is expandable by connecting additional components
 * to the existing pin header. Several digitial I/Os, A/D inputs as well
 * as the standard SPI and I2C (TWI) serial interfaces are available for
 * user-defined purposes.
 *
 * The curcuit board is designed as a two-layer board of size 100mm x 80mm. Most
 * components use SMD packages.
 *
 * \image html mega-eth_rev10.jpg "The mega-eth board running with a memory card and ethernet connected."
 * \image html mega-eth_rev10_layout.png "The layout of the revision 1.0 circuit board."
 *
 * \section software The software
 * The software is written in pure standard ANSI-C. Sure, it might not be the
 * smallest or the fastest one, but I think it is quite flexible.
 *
 * By providing the software for this project under the GPLv2, great
 * flexibility is achieved. The following modules are already available:
 * - \link net_stack TCP/IP and UDP/IP stack \endlink.
 * - \link app_dhcp_client Client DHCP support \endlink for automatically configuring the network interface.
 * - \link app_httpd Small and flexible web server \endlink.
 * - \link sd Mature and well-tested SD/MMC access with FAT16 support \endlink.
 * - Client for retrieving current \link app_clock_sync date and time over network (TIME protocol) \endlink.
 * - Several internal modules for providing a \link sys_clock clock \endlink, \link sys_timer timers \endlink, \link arch_uart UART access \endlink and more.
 *
 * I implemented a simple command prompt which is accessible via the UART at 115200 Baud. With
 * commands similiar to the Unix shell you can setup the network parameters, browse the SD/MMC
 * memory card and more.
 * - <tt>cat \<file\></tt>\n
 *   Writes a hexdump of <tt>\<file\></tt> to the terminal.
 * - <tt>cd \<directory\></tt>\n
 *   Changes current working directory to <tt>\<directory\></tt>.
 * - <tt>clock</tt>\n
 *   Prints date and time from the internal clock.
 * - <tt>clock_sync_start</tt>\n
 *   Starts periodically fetching current date and time from the internet.
 *   \note In the official firmware my own server is contacted. This is for
 *		 illustration purposes only. The service is not guaranteed and should
 *		 not be used more often than every ten minutes!
 * - <tt>clock_sync_stop</tt>\n
 *   Aborts clock synchronization.
 * - <tt>disk</tt>\n
 *   Shows card manufacturer, status, filesystem capacity and free storage space.
 * - <tt>dhcp_client_start</tt>\n
 *   Starts the DHCP client and tries to configure the network interface.
 * - <tt>dhcp_client_abort</tt>\n
 *   Aborts the DHCP client.
 * - <tt>ip_config [\<ip_address\> [\<ip_netmask\> [\<ip_gateway\>]]]</tt>\n
 *   Displays or changes the network interface configuration.
 * - <tt>ls</tt>\n
 *   Shows the content of the current directory.
 * - <tt>sync</tt>\n
 *   Ensures all buffered data is written to the card.
 *
 * \section adaptation Adapting the software to your needs
 * The software has been written with reusability in mind. Most modules are hardware independent,
 * and hardware abstraction has been used where hardware access is needed. That said, it should
 * be fairly easily adaptible to different hardware configurations.
 *
 * By changing the <tt>MCU*</tt> variables in the Makefile, you can use other Atmel
 * microcontrollers or different clock speeds. Some modules are configurable, the parameters reside
 * in extra files named for example <tt>tcp_config.h</tt> or <tt>ethernet_config.h</tt>.
 * 
 * \section bugs Bugs or comments?
 * If you have comments or found a bug in the software - there might be some
 * of them - you may contact me per mail at feedback@roland-riegel.de.
 *
 * \section copyright Copyright 2006-2008 by Roland Riegel
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).
 * At your option, you can alternatively redistribute and/or modify the following
 * files under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation (http://www.gnu.org/copyleft/lgpl.html):
 * - fat16.c
 * - fat16.h
 * - fat16_config.h
 * - partition.c
 * - partition.h
 * - partition_config.h
 * - sd_raw.c
 * - sd_raw.h
 * - sd_raw_config.h
 */

/*
		The^day^of^DooM

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
	Last Update:	16.09.2011
	Last Update:	15.01.2012
	Full Change:	02.02.2012 - For Board v.0.0.5 (10.01.2012)
	Last Update:	21.02.2012


		Web Server, ModBUS TCP and other
			www.roland-riegel.de

	Create Date:	xx.10.2012 - For Board v.0.0.7 (14.10.2012)
	Last Update:	16.10.2012
	Full Change:	03.01.2013
	Last Update:	19.01.2013
	Last Update:	26.02.2013
	Last Update:	04.03.2013
	Last Update:	05.03.2013
	Last Update:	24.11.2013
	Last Update:	29.11.2013

		DC Servo, add project "I2C Servo Controller"
			www.franksworkshop.com.au

	Create Date:	23.08.2013
	Full Change:	23.08.2013
	Last Update:	23.08.2013
*/

#include "main.h"

/* ----------------------- Modbus includes ----------------------------------------------- */
#include "mb.h"
#include "mbport.h"
#include "mbutils.h"

/* ----------------------------------- Defines ----------------------------------------------
		Байтова карта на дискретните входове:

	BYTE[0] - Цифрови входове:
		BIT[0:7]	- Входове [I0:I7]

	BYTE[1] - Цифрови входове:
		BIT[0:7]	- Входове [I8:I15]

	BYTE[2] - Цифрови входове:
		BIT[0:7]	- Входове [I16:I23]

	BYTE[3] - Цифрови входове:
		BIT0		- Защита на изходите
*/
// MB_FUNC_READ_DISCRETE_INPUTS					( 2 )
#define REG_DISC_START							1
#define REG_DISC_SIZE							32

unsigned char ucRegDiscBuf[REG_DISC_SIZE / 8] = { 0, 0 };

/* ------------------------------------------------------------------------------------------
		Байтова карта на бобините за четене/запис:

	BYTE[0] - Цифрови изходи:
		BIT[0:7]	- Изходи [Q0:Q7]

	BYTE[1] - Цифрови изходи:
		BIT[0:7]	- Изходи [Q8:Q15]

	BYTE[2] - Цифрови изходи:
		BIT[0:7]	- Изходи [Q16:Q23]
*/
// MB_FUNC_READ_COILS							( 1 )
// MB_FUNC_WRITE_SINGLE_COIL					( 5 )
// MB_FUNC_WRITE_MULTIPLE_COILS					( 15 )
#define REG_COILS_START							1
#define REG_COILS_SIZE							24

uint8_t ucRegCoilsBuf[REG_COILS_SIZE / 8];

/* ------------------------------------------------------------------------------------------
		Байтова карта на регистрите само за четене:

	WORD_0[0:1]:	[LSB:MSB] Аналогов вход 0, без корекция					(0 - 2047);
	WORD_1[2:3]:	[LSB:MSB] Аналогов вход 1, без корекция					(0 - 2047);
								=//=
	WORD_6[12:13]:	[LSB:MSB] Аналогов вход 6, без корекция					(0 - 2047);
	-----------------------------------------------------------------------------------------
	WORD_7[14:15]:	[LSB:MSB] Аналогов вход 0, в диапазона: 0.00V - 10.00V		(0 - 1000);
	WORD_8[16:17]:	[LSB:MSB] Аналогов вход 1, в диапазона: 0.00V - 10.00V		(0 - 1000);
								=//=
	WORD_13[26:27]:	[LSB:MSB] Аналогов вход 6, в диапазона: 0.00V - 10.00V		(0 - 1000);
	-----------------------------------------------------------------------------------------
	WORD_14[28:29]:	[LSB:MSB] Аналогов вход 0 и 1, в диапазона: 0.0V - 10.0V	(0 - 100);
	WORD_15[30:31]:	[LSB:MSB] Аналогов вход 2 и 3, в диапазона: 0.0V - 10.0V	(0 - 100);
	WORD_16[32:33]:	[LSB:MSB] Аналогов вход 4 и 5, в диапазона: 0.0V - 10.0V	(0 - 100);
	WORD_17[34:35]:	[LSB:MSB] Аналогов вход 6 и 7, в диапазона: 0.0V - 10.0V	(0 - 100);
	-----------------------------------------------------------------------------------------
	WORD_18[36:37]:	[LSB:MSB] - MSB: DIP8 Switch
	-----------------------------------------------------------------------------------------
	WORD_19[38:39]: [LSB0:MSB1] Optical Encoder COUNTER
	WORD_20[40:41]: [LSB2:MSB3] Optical Encoder COUNTER

*/
// MB_FUNC_READ_INPUT_REGISTER					(  4 )
#define REG_INPUT_START							1
#define REG_INPUT_NREGS							42

uint16_t uiRegInputBuf[REG_INPUT_NREGS];
uint8_t usRegInputStart = REG_INPUT_START;

/* ------------------------------------------------------------------------------------------
		Байтова карта на регистрите за четене/запис:

	WORD_0[0:1]:	[LSB:MSB] Аналогов изход 0, без корекция в диапазона		(0 - 65535);
	WORD_1[2:3]:	[LSB:MSB] Аналогов изход 1, без корекция в диапазона		(0 - 65535);
	WORD_2[4:5]:	[LSB:MSB] Аналогов изход 0, в диапазона: 0.00V - 10.00V	(0 - 1000);
	WORD_3[6:7]:	[LSB:MSB] Аналогов изход 1, в диапазона: 0.00V - 10.00V	(0 - 1000);
	WORD_4[8:9]:	[LSB:MSB] Аналогов изход 0 и 1, в диапазона: 0.0V - 10.0V	(0 - 100);
	-----------------------------------------------------------------------------------------
	WORD_5[10:11]:	[LSB:MSB] Калибрираща константа на Аналогов вход 0
	WORD_6[12:13]:	[LSB:MSB] Калибрираща константа на Аналогов вход 1
								=//=
	WORD_11[22:23]:	[LSB:MSB] Калибрираща константа на Аналогов вход 6
	-----------------------------------------------------------------------------------------
	WORD_12[24:25]:	[LSB:MSB] Калибрираща константа на Аналогов изход 0
	WORD_13[26:27]:	[LSB:MSB] Калибрираща константа на Аналогов изход 1
	-----------------------------------------------------------------------------------------
	WORD_14[28:29]:	[LSB:MSB] Контролни битове на аналоговите входове/изходи:
		BYTE[28] - Аналогови входове:
			BIT[0:1] - Избор на входна точка: (НЕ СЕ ПОЛЗВАТ!!! МОГАТ ДА СЕ ЧЕТАТ ПРОИЗВОЛНО)
				0 - Без промяна на предишното състояние
				1 - Пълен диапазон				(0 - 2047)
				2 - Диапазон 0.00 V - 10.00 V	(0 - 1000)
				3 - Диапазон 0.0 V - 10.0 V		(0 - 100)

			BIT 5 - Зареждане на калибриращите константи по подразбиране
			BIT 6 - Зареждане на калибриращите константи от EEPROM
			BIT 7 - Зареждане на калибриращите константи в EEPROM

		BYTE[29] - Аналогови изходи:
			BIT[0:1] - Избор на входна точка:
				0 - Без промяна на предишното състояние
				1 - Пълен диапазон				(0 - 2047)
				2 - Диапазон 0.00 V - 10.00 V	(0 - 1000)
				3 - Диапазон 0.0 V - 10.0 V		(0 - 100) - NA!!!

			BIT 5 - Зареждане на калибриращите константи по подразбиране
			BIT 6 - Зареждане на калибриращите константи от EEPROM
			BIT 7 - Зареждане на калибриращите константи в EEPROM
	-----------------------------------------------------------------------------------------
	WORD_15[30:31]:	[LSB:MSB]; Контролни за състояние на модула
		BYTE[30] - Статусни битове:
			0 - RUN/!STOP
			1 - Термична защита
			2 - Задействана защита по time out на комуникацията.
				Модула автоматично минава в стоп режим от който се излиза
				след подаване на команда RUN (ръчно или по ModBUS), при което
				бита се изчиства до следващия time out.
	-----------------------------------------------------------------------------------------
	WORD_16[32:33]:	[LSB:MSB]; Младша дума на BAUD Rate
	WORD_17[36:37]:	[LSB:MSB]; Контролен регистър на UART:
		BYTE[36] - Контролен Byte (LSB):
			BIT[0:1] - Избор на "Parity":
				0 - Без промяна на предишното състояние
				1 - Even
				2 - Odd
				3 - None

			BIT[2:3] - Избор на сериен порт:
				0 - Без промяна на предишното състояние
				4 - RS485
				8 - FT232 (USB Serial)

		BYTE[37] - Контролен Byte (MSB):
			BIT 4 - Зареждане на новите настроики
			BIT 5 - Зареждане на настроиките по подразбиране
			BIT 6 - Зареждане на настроиките от EEPROM
			BIT 7 - Зареждане на настроиките в EEPROM

	WORD_18[38:39]:	[LSB:MSB]; ModBUS Time Out (0 - Disable this function)
	WORD_19[40:41]:	[LSB:MSB]; ModBUS Frame Time Out
	-----------------------------------------------------------------------------------------
	WORD_20[42:43]:	[LSB:MSB]; Контролни битове за четене от EEPROM
	-----------------------------------------------------------------------------------------
	WORD_21[44:45]:	[LSB:MSB]; Контролни битове за запис в EEPROM
	-----------------------------------------------------------------------------------------
*/
// MB_FUNC_WRITE_REGISTER						( 6 )
// MB_FUNC_READ_HOLDING_REGISTER				( 3 )
// MB_FUNC_WRITE_MULTIPLE_REGISTERS				( 16 )
// MB_FUNC_READWRITE_MULTIPLE_REGISTERS			( 23 )
#define REG_HOLDING_START						1
#define REG_HOLDING_NREGS						100

uint16_t uiRegHolding[REG_HOLDING_NREGS];

/* --------------------------------- Extern variables ------------------------------------ */
extern volatile uint8_t bDoPID;
extern volatile int16_t nMaxPidOut;
extern volatile int32_t nEncoderPosition;

/* --------------------------------- Static variables ------------------------------------ */
static volatile bool net_link_up = 0;
static volatile uint16_t timer_events = 0;
static volatile int32_t nEncoderPositionOld = 0;

/* --------------------------------- Other varitables ------------------------------------ */
volatile uint8_t mac_addr[6] = { 'F', 'O', 'O', 'B', 'A', 'R' };

/*
	inPort[0..9]	- Digital input: ID[0..9]
	inPort[10]		- Power supply for digital output FB: OUT_FB_VCC
	inPort[11..13]	- Digital output FB: OUT_FB0[0..2]
	inPort[14..15]	- Jumper: SJ901 and SJ902
*/
volatile uint8_t inPort[16], outPort[12];
volatile uint16_t arrADC[7], arrDAC[2];

volatile pidData_t pidPosData;
volatile int32_t p_factor, i_factor, d_factor;
extern volatile uint16_t SCALING_FACTOR;
extern volatile uint16_t MAX_I_TERM;

EEMEM uint8_t firstRunEEPROM;
EEMEM uint8_t isRunEEPROM;

EEMEM uint16_t arrADCConstEEPROM[7];
EEMEM uint16_t arrDACConstEEPROM[2];
EEMEM uint32_t analogControlWordEEPROM;

EEMEM uint16_t uwBaudRateEEPROM;
EEMEM uint16_t uiUartControlEEPROM;
EEMEM uint16_t uiModBusTimeOutEEPROM;

/* ------------------------------- Start implementation ---------------------------------- */
static void dhcp_client_event_callback(enum dhcp_client_event event);

int main()
{
	if( 0 ) {
		int32_t out, pv = 0, sp = 0;

		p_factor = 30;
		i_factor = 5;
		d_factor = 10;
		pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );

		while( 1 ) {
			sp += 2;
			pv += 1;

			out = pid_Controller( pv, sp, (pidData_t*)&pidPosData );

			PORTA = (uint8_t)out;
		}
	}

	if( 0 ) {
		TCCR2 |= (1<<WGM21) | (0<<WGM20) | (1<<CS22) | (0<<CS21) | (1<<CS20);
		TIMSK |= (1<<OCIE2);
		OCR2 = F_CPU / 1024 / 1000;

		servoInit( );

		sei();
		while( 1 ) {
			volatile motion_t buf[3] = { 0 };

			switch( buf[0] ) {
			case 1:
				SetMaxSpeed( buf[1] );
			 break;

			case 2:
				SetAcceleration( buf[1] );
			 break;
			
			case 3:
				SetVelocity( buf[1] );
			 break;

			case 4:
				SetAccAndMaxVelocity( buf[1], buf[2] );
			 break;

			case 5:
				Move( buf[1] );
			 break;

			case 6:
				MoveTo( buf[1] );
			 break;
			}

			MotionUpdate();
		}
	}

	char rs485_address_switch_old = 0;
	struct fat16_dir_struct* dir = 0;
	int i, n = 0;
	char fb = 0;

	wdt_enable(WDTO_2S);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PORTB = CPU_CS_NET_bm | CPU_CS3_bm | CPU_CS2_bm | CPU_CS1_bm | CPU_CS0_bm;
	DDRB = CPU_CS_NET_bm | CPU_CS3_bm | CPU_CS2_bm | CPU_CS1_bm | CPU_CS0_bm;

	PORTD = CPU_CS_RS485_ADDRESS_bm | CPU_CS_AO_bm;
	DDRD = CPU_CS_RS485_ADDRESS_bm | RS485_DE_bm | CPU_CS_AO_bm;

	PORTE = CPU_CS_DO_bm | CPU_CS_DI_bm;
	DDRE = CPU_CS_DO_bm | CPU_CS_DI_bm;

	PORTF = CPU_CS_AI_bm;
	DDRF = CPU_IO_RESET_bm | RS485_DE_bm | CPU_CS_AI_bm;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	unselect_analog_in();
	unselect_analog_out();
	unselect_digital_in();
	unselect_digital_out();
	unselect_address_switch();

	//disable_rs485_transmit();

	enable_io_reset();
	_delay_ms(10);
	disable_io_reset();
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	memset(&arrADC, 0, sizeof(arrADC));
	memset(&arrDAC, 0, sizeof(arrDAC));
	memset(&inPort, 0, sizeof(inPort));
	memset(&outPort, 0, sizeof(outPort));

	memset(&ucRegDiscBuf, 0, sizeof(ucRegDiscBuf));
	memset(&ucRegCoilsBuf, 0, sizeof(ucRegCoilsBuf));
	memset(&uiRegInputBuf, 0, sizeof(uiRegInputBuf));
	memset(&uiRegHolding, 0, sizeof(uiRegHolding));
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	sei();
	/*uart_init();
	uart_connect_stdio();
	printf_P( PSTR("mega-eth booting...\n") );*/
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	spi_init();
	initAddressSwitch();
	initDigitalInput((uint8_t*)inPort);
	initDigitalOutput((uint8_t*)outPort);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	hal_init((unsigned char*)mac_addr);
	// initialize ethernet protocol stack
	ethernet_init((unsigned char*)mac_addr);
	arp_init();
	ip_init(0, 0, 0);
	icmp_init();
	tcp_init();
	udp_init();
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(	//!(PROGPIN & (1<<PROG_NO)) ||						// Load Def. Setings
		0xAA != eeprom_read_byte((void*)&firstRunEEPROM)	// First Run!!!
	) {
		//videoPutStrXY(3, 0, (unsigned char *)"AVR-IO-MODULE");
		//videoPutStrXY(2, 2, (unsigned char *)"Please wait while");
		//videoPutStrXY(3, 3, (unsigned char *)"init all Obj's.");

		//isRun = 128 | 2 | 1;
		uiRegHolding[15] = 1;

		//uartLoadDefConst();
		adcLoadDefConst( &uiRegHolding[5] );

		uiRegHolding[14] = 0x0f<<8;
		dacLoadDefConst(&uiRegHolding[12]);

#ifndef __DEBUG__
		//eeprom_update_byte((void*)&isRunEEPROM, isRun);
		
		//eeprom_update_word((void*)&uwBaudRateEEPROM, uiRegHolding[16]);
		//eeprom_update_word((void*)&uiUartControlEEPROM, uiRegHolding[17]);
		//eeprom_update_word((void*)&uiModBusTimeOutEEPROM, uiRegHolding[18]);

		eeprom_update_word((void*)&analogControlWordEEPROM, uiRegHolding[14]);
		eeprom_update_block((void*)&uiRegHolding[5], (void*)&arrADCConstEEPROM, sizeof(arrADCConstEEPROM));
		eeprom_update_block((void*)&uiRegHolding[12], (void*)&arrDACConstEEPROM, sizeof(arrDACConstEEPROM));

		//if(PROGPIN & (1<<PROG_NO)) {
			eeprom_update_byte((void*)&firstRunEEPROM, 0xAA);
		//}
		//_delay_ms(1000);
#endif
	} else {
		//isRun = eeprom_read_byte((void*)&isRunEEPROM) | 128;
		//uiRegHolding[15] = 1 & isRun;

		uiRegHolding[16] = eeprom_read_word((void*)&uwBaudRateEEPROM);
		uiRegHolding[17] = eeprom_read_word((void*)&uiUartControlEEPROM);
		uiRegHolding[18] = eeprom_read_word((void*)&uiModBusTimeOutEEPROM);

		eeprom_read_block((void*)&uiRegHolding[5], (void*)&arrADCConstEEPROM, sizeof(arrADCConstEEPROM));
		eeprom_read_block((void*)&uiRegHolding[12], (void*)&arrDACConstEEPROM, sizeof(arrDACConstEEPROM));
		eeprom_read_block((void*)&uiRegHolding[14], (void*)&analogControlWordEEPROM, sizeof(*uiRegHolding));
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// setup interval timer
	TCCR2 |= (1<<WGM21) | (0<<WGM20) | (1<<CS22) | (0<<CS21) | (1<<CS20);
	TIMSK |= (1<<OCIE2);
	//OCR2 = F_CPU / 1024 / 100;
	OCR2 = F_CPU / 1024 / 1000;

	// start clock
	clock_init();

	// start http server
	httpd_init(88);

	int eStatus;
	eStatus = eMBTCPInit(502);
	//eStatus = eMBInit( MB_RTU, 10, 0, 115200, MB_PAR_EVEN );
	if( MB_ENOERR == eStatus ) {
		eMBEnable();
	} else {
		while( 1 );
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*SCALING_FACTOR = 1000;
	MAX_I_TERM = 5000;
	
	p_factor = (int32_t)( 0.50f * (float)SCALING_FACTOR );
	i_factor = (int32_t)( 0.01f * (float)SCALING_FACTOR );
	d_factor = (int32_t)( 0.50f * (float)SCALING_FACTOR );*/

	SCALING_FACTOR = 256;
	MAX_I_TERM = 200;

	p_factor = 50;
	i_factor = 5;
	d_factor = 10;

	uiRegHolding[66] = 125;

	uiRegHolding[52] = MAX_I_TERM;
	uiRegHolding[53] = SCALING_FACTOR;

	uiRegHolding[49] = p_factor;
	uiRegHolding[50] = i_factor;
	uiRegHolding[51] = d_factor;
	pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	servoInit( );
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	while( 1 ) {
		wdt_reset();

		if(0x1000 & uiRegHolding[14]) {
			uiRegHolding[14] &= ~0x1000;
			eeprom_update_word((void*)&analogControlWordEEPROM, 0x0f00 & uiRegHolding[14]);
		}

		if( 0xE0E0 & uiRegHolding[14] ) {
			if(0x0020 & uiRegHolding[14]) {
				uiRegHolding[14] &= ~0x0020;
				adcLoadDefConst(&uiRegHolding[5]);
			}

			// Аналогови входове, зареждане на калибриращите
			// константи от EEPROM:
			if(0x0040 & uiRegHolding[14]) {
				uiRegHolding[14] &= ~0x0040;
				eeprom_read_block((void*)&uiRegHolding[5], (void*)&arrADCConstEEPROM, sizeof(arrADCConstEEPROM));
			}

			// Аналогови входове, зареждане на калибриращите
			// константи в EEPROM:
			if(0x0080 & uiRegHolding[14]) {
				uiRegHolding[14] &= ~0x0080;
				eeprom_update_block((void*)&uiRegHolding[5], (void*)&arrADCConstEEPROM, sizeof(arrADCConstEEPROM));
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			if(0x2000 & uiRegHolding[14]) {
				uiRegHolding[14] &= ~0x2000;
				dacLoadDefConst(&uiRegHolding[12]);
			}

			// Аналогови изходи, зареждане на калибриращите
			// константи от EEPROM:
			if(0x4000 & uiRegHolding[14]) {
				uiRegHolding[14] &= ~0x4000;
				eeprom_read_block((void*)&uiRegHolding[12], (void*)&arrDACConstEEPROM, sizeof(arrDACConstEEPROM));
			}

			// Аналогови изходи, зареждане на калибриращите
			// константи в EEPROM:
			if(0x8000 & uiRegHolding[14]) {
				uiRegHolding[14] &= ~0x8000;
				eeprom_update_block((void*)&uiRegHolding[12], (void*)&arrDACConstEEPROM, sizeof(arrDACConstEEPROM));
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		readADC();

		for(i = 0; i < 7; i++) {
		//	uiRegInputBuf[7 + i] = (10000.0 * (float)arrADC[i]) / ((float)uiRegHolding[5 + i]);
		}

		n = 0;
		for(i = 14; i < 18; i++) {
			uiRegInputBuf[i] = 0x00ff & (uiRegInputBuf[++n] / 10);
			if( ++n < 14 ) {
				uiRegInputBuf[i] |= (0xff00 & ((uiRegInputBuf[n] / 10)<<8));
			}
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		readDigitalInput((uint8_t*)inPort);

		uiRegInputBuf[18] &= 0xff00;
		uiRegInputBuf[18] |= readAddressSwitch();

		n = 0;
		for(i = 0; i < 15; i++) {
			if(i > 7) n = 1;

			if(inPort[i]) {
				ucRegDiscBuf[n] |=  (1<<(i - 8 * n));
			} else {
				ucRegDiscBuf[n] &= ~(1<<(i - 8 * n));
			}
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Handle sd card plugging:
		if( !sd_raw_available() && sd_get_root_dir() ) {
			fat16_close_dir(dir);
			sd_close();
			dir = 0;
			printf_P(PSTR("[sd] card unplugged\n"));
		} else {
			if( !sd_get_root_dir() && sd_raw_available() ) {
				//spi_init();
				sd_raw_init();

				switch(sd_open()) {
				case SD_ERROR_NONE:
					dir = sd_get_root_dir();
					printf_P(PSTR("[sd] card initialized\n"));
				 break;

				case SD_ERROR_FS: printf_P(PSTR("[sd] opening filesystem failed\n")); break;
				case SD_ERROR_INIT: printf_P(PSTR("[sd] MMC/SD initialization failed\n")); break;
				case SD_ERROR_PARTITION: printf_P(PSTR("[sd] opening partition failed\n")); break;
				case SD_ERROR_ROOTDIR: printf_P(PSTR("[sd] opening root directory failed\n")); break;

				default: printf_P(PSTR("[sd] unknown error\n")); break;
				}
			}
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if( rs485_address_switch_old != uiRegInputBuf[18] ) {
			eMBDisable();

			if( 64 & uiRegInputBuf[18] ) {
				eStatus = eMBTCPInit( 502 );
			} else {
				eStatus = eMBInit( MB_RTU, 10, 0, 115200, MB_PAR_EVEN );
			}

			if( MB_ENOERR == eStatus ) {
				eMBEnable();
			} else {
				asm("nop\n");
			}
		}

		/* handle network link changes */
		if( //( rs485_address_switch_old != uiRegInputBuf[18] ) ||
			( !net_link_up && hal_link_up() )
		) {
			if( 0x0080 & uiRegInputBuf[18] ) {
				dhcp_client_start(dhcp_client_event_callback);
			} else {
				uint8_t ip_addr[4] = { 10, 0, 2, 76 };
				uint8_t netmask[4] = { 255, 255, 255, 0 };
				uint8_t gateway[4] = { 10, 0, 2, 1 };

				ip_init(
					(unsigned char*)ip_addr,
					(unsigned char*)netmask,
					(unsigned char*)gateway
				);
			}

			net_link_up = 1;
		} else {
			if( ( net_link_up && !hal_link_up() )
			) {
				if( 0x0080 & uiRegInputBuf[18] ) {
					dhcp_client_abort();
				}

				ip_init(0, 0, 0);

				net_link_up = 0;
			}
		}

		rs485_address_switch_old = uiRegInputBuf[18];
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Receive packets:
		while( ethernet_handle_packet() && !bDoPID );
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Handle timeouts:
		cli();
		uint8_t timer_ticks = 20; // timer_events;
		timer_events = 0;
		sei();

		while( timer_ticks-- ) {
			timer_interval();
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		uiRegInputBuf[39] = nEncoderPositionOld ;
		uiRegInputBuf[40] = nEncoderPositionOld>>16;
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		eMBPoll();
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		fb = 0;
		if( outPort[0] ) {
			if( bDoPID ) {
				int16_t dac, SpeedLimit;
				motion_t nNewPosition;
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				cli();
				bDoPID = 0;
				nEncoderPositionOld = nEncoderPosition;
				sei();
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				//nNewPosition = ((int32_t)(uiRegHolding[56])<<16 | uiRegHolding[55]);
				nNewPosition = motionGetCurrentPosition() / NUMBER_SCALE;
				SpeedLimit = uiRegHolding[66];
				////////////////////////////////////////////////////////////////////////////////
				dac = pid_Controller( nNewPosition, nEncoderPositionOld, (pidData_t*)&pidPosData );
				if( dac < 0 ) {
					dac = -dac;
					fb = 1;
				}
				if( dac > SpeedLimit ) {
					dac = SpeedLimit;
				}
				arrDAC[0] = dac * 510;
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				MotionUpdate();
			}
		} else {
			pid_Reset_Integrator( (pidData_t*)&pidPosData );
			uiRegHolding[56] = uiRegHolding[55] = 0;
			arrDAC[0] = 0;

			motionSetCurrentPosition( 0 );
			motionSetTargetPosition( 0 );
			motionSetCurrentVelocity( 0 );
			motionSetRunState( eStopped );

			cli();
			nEncoderPosition = 0;
			sei();
		}

		if( 1 ) { //!((64 | 32) & isRun) && (1 & isRun) ) {
			for(i = 0; i < 12; i++) {
				if( i != 3 ) {
					if(i < 8) {
						if((1<<i) & ucRegCoilsBuf[0]) outPort[i] = 1;
						else outPort[i] = 0;
					} else {
						if((1<<(i-8)) & ucRegCoilsBuf[1]) outPort[i] = 1;
						else outPort[i] = 0;
					}
				}
			}

			switch(0x03 & (uiRegHolding[14]>>10)) {
			case 1:
				arrDAC[1] = uiRegHolding[1];
			 break;

			case 2:
				arrDAC[1] = ((float)uiRegHolding[13]/1000.0) * (float)uiRegHolding[3];
			 break;
			}

			switch(0x03 & (uiRegHolding[14]>>8)) {
			case 1:
				//arrDAC[0] = uiRegHolding[0];
			 break;

			case 2:
				//arrDAC[0] = ((float)uiRegHolding[12]/1000.0) * (float)uiRegHolding[2];
			 break;
			}
		} else {
			memset( &arrDAC, 0, sizeof(arrDAC) );
			memset( &uiRegHolding, 0, 4 * sizeof(*uiRegHolding) );

			memset( &outPort, 0, sizeof(outPort) );
			memset( &ucRegCoilsBuf, 0, sizeof(ucRegCoilsBuf) );
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		outPort[3] = fb;
		if( !fb ) {
			ucRegCoilsBuf[0] &= ~8;
		} else {
			ucRegCoilsBuf[0] |=  8;
		}
		writeDigitalOutput( (uint8_t*)outPort );
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		writeDac( arrDAC[0], arrDAC[1] );
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
	
	while(1);
}

// Fires 1000 times a second / every 1ms:
ISR( TIMER2_COMP_vect )
{
	volatile static uint8_t n = 0;

	if( 2 == ++n ) {
		++timer_events;
		n = 0;
	}

	bDoPID = 1;
}

void dhcp_client_event_callback(enum dhcp_client_event event)
{
	switch(event) {
	case DHCP_CLIENT_EVT_LEASE_ACQUIRED:
		printf_P(PSTR("[dhcp] lease acquired\n"));
	 break;
	case DHCP_CLIENT_EVT_LEASE_DENIED:
		printf_P(PSTR("[dhcp] lease denied\n"));
	 break;
	case DHCP_CLIENT_EVT_LEASE_EXPIRING:
		printf_P(PSTR("[dhcp] lease expiring\n"));
	 break;
	case DHCP_CLIENT_EVT_LEASE_EXPIRED:
		printf_P(PSTR("[dhcp] lease expired\n"));
	 break;
	case DHCP_CLIENT_EVT_TIMEOUT:
		printf_P(PSTR("[dhcp] timeout\n"));
	 break;
	case DHCP_CLIENT_EVT_ERROR:
		printf_P(PSTR("[dhcp] error\n"));
	 break;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initDigitalInput(uint8_t in[16])
{
	memset(in, 0, sizeof(in));

	select_digital_in();
	mcp23s08_ddr_init(0x00);
	unselect_digital_in();

	select_digital_in();
	mcp23s08_input_polarity(0x3F);
	unselect_digital_in();	
}

void readDigitalInput(uint8_t in[16])
{
	uint8_t i, temp, outa, outb;

	select_digital_in();
	temp = mcp23sxx_spi_read(MCP23S08_GPIO);
	unselect_digital_in();

	select_digital_out();
	outa = mcp23sxx_spi_read(MCP23S17_GPIOA);
	unselect_digital_out();

	select_digital_out();
	outb = mcp23sxx_spi_read(MCP23S17_GPIOB);
	unselect_digital_out();
	
	// ID[0..5]
	for(i = 0; i < 6; i++) {
		in[i] = (temp & (1<<i)) ? 1 : 0;
	}

	in[6] = (PIND & ID6_IC1_bm)  ? 0 : 1; // ID6
	in[7] = (PINE & ID7_INT6_bm) ? 0 : 1; // ID7
	in[8] = (PINE & ID8_INT5_bm) ? 0 : 1; // ID8
	in[9] = (PINE & ID9_INT4_bm) ? 0 : 1; // ID9

	in[10] = (0x80 & outb) ? 1 : 0;       // OUT_FB_VCC
	in[11] = (0x40 & outb) ? 1 : 0;       // OUT_FB0
	in[12] = (0x02 & outb) ? 1 : 0;       // OUT_FB1
	in[13] = (0x10 & outa) ? 1 : 0;       // OUT_FB2

	in[14] = (0x40 & temp) ? 1 : 0;       // SJ901
	in[15] = (0x80 & temp) ? 1 : 0;       // SJ902
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initDigitalOutput(uint8_t out[12])
{
	memset(out, 0, sizeof(out));

	select_digital_out();
	mcp23sxx_spi_write(MCP23S17_IODIRA, (unsigned char)~0xef);
	unselect_digital_out();

	select_digital_out();
	mcp23sxx_spi_write(MCP23S17_IODIRB, (unsigned char)~0x3d);
	unselect_digital_out();
}

void writeDigitalOutput(uint8_t out[12])
{
	int i;
	unsigned int gpio = 0;
	static unsigned int digitalOutMap[] = {
		0x0400,	// DO0	<> GPB2
		0x0800,	// DO1	<> GPB3
		0x1000, // DO2	<> GPB4
		0x2000, // DO3	<> GPB5
		0x0020, // DO4	<> GPA5
		0x0040, // DO5	<> GPA6
		0x0080, // DO6	<> GPA7
		0x0100, // DO7	<> GPB0
		0x0001,	// DO8	<> GPA0
		0x0002, // DO9	<> GPA1
		0x0004, // DO10 <> GPA2
		0x0008  // DO11 <> GPA3
	};

	for(i = 0; i < 12; i++) {
		if(out[i]) {
			gpio |= digitalOutMap[i];
		}
	}

	select_digital_out();
	mcp23s17_write_outa((char)gpio);
	unselect_digital_out();

	select_digital_out();
	mcp23s17_write_outb((char)(gpio>>8));
	unselect_digital_out();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initAddressSwitch(void)
{
	select_address_switch();
	mcp23s08_ddr_init(0x00);
	unselect_address_switch();

	select_address_switch();
	mcp23s08_input_polarity(0xff);
	unselect_address_switch();
}

char readAddressSwitch(void)
{
	char temp;

	select_address_switch();
	temp = mcp23sxx_spi_read(MCP23S08_GPIO);
	unselect_address_switch();

	return temp;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
eMBErrorCode eMBRegDiscreteCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
	volatile eMBErrorCode eStatus = MB_ENOERR;
	volatile short iNDiscrete = ( short )usNDiscrete;
	volatile unsigned short usBitOffset;

	// MB_FUNC_READ_DISCRETE_INPUTS          ( 2 )
	/* Check if we have registers mapped at this block. */
	if( (usAddress >= REG_DISC_START) &&
		(usAddress + usNDiscrete <= REG_DISC_START + REG_DISC_SIZE)
	) {
		usBitOffset = ( unsigned short )( usAddress - REG_DISC_START );
		while(iNDiscrete > 0) {
			*pucRegBuffer++ =
			xMBUtilGetBits( ucRegDiscBuf, usBitOffset,
                            (unsigned char)(iNDiscrete>8? 8:iNDiscrete)
			);
			iNDiscrete -= 8;
			usBitOffset += 8;
		}
		return MB_ENOERR;
	}
	
	return eStatus;
}

eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
							USHORT usNCoils, eMBRegisterMode eMode
						  )
{
    short           iNCoils = ( short )usNCoils;
    unsigned short  usBitOffset;
	
	/* Check if we have registers mapped at this block. */
	if( (usAddress >= REG_COILS_START) &&
		(usAddress + usNCoils <= REG_COILS_START + REG_COILS_SIZE)
	) {
		usBitOffset = (unsigned short)(usAddress - REG_COILS_START);
		switch(eMode) {
		/*
			Read current values and pass to protocol stack.
			MB_FUNC_READ_COILS						( 1 )
		*/
		case MB_REG_READ:
			while( iNCoils > 0 ) {
				*pucRegBuffer++ =
				xMBUtilGetBits( ucRegCoilsBuf, usBitOffset,
								(unsigned char)((iNCoils > 8) ? 8 : iNCoils)
				);
				usBitOffset += 8;
				iNCoils -= 8;
			}
		 return MB_ENOERR;
		 
		 /*
		 	Update current register values.
		 	MB_FUNC_WRITE_SINGLE_COIL				( 5 )
			MB_FUNC_WRITE_MULTIPLE_COILS			( 15 )
		 */
		 case MB_REG_WRITE:
		 	while( iNCoils > 0 ) {
				xMBUtilSetBits( ucRegCoilsBuf, usBitOffset,
								(unsigned char)((iNCoils > 8) ? 8 : iNCoils),
								*pucRegBuffer++
				);
				usBitOffset += 8;
				iNCoils -= 8;
			}
		 return MB_ENOERR;
		}
	}
	
	return MB_ENOREG;
}

eMBErrorCode eMBRegInputCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
	unsigned int iRegIndex;
 
	// MB_FUNC_READ_INPUT_REGISTER           (  4 )
	if( (usAddress >= REG_INPUT_START) &&
		(usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS)
	) {
		/*for(int i = 0; i < 7; i++) {
			uiRegInputBuf[i] += i;
		}*/

		iRegIndex = (int)(usAddress - usRegInputStart);
		while( usNRegs > 0 ) {
			*pucRegBuffer++ = (unsigned char)(uiRegInputBuf[iRegIndex] >> 8);
            *pucRegBuffer++ = (unsigned char)(uiRegInputBuf[iRegIndex] & 0xFF);
			++iRegIndex;
			--usNRegs;
		}
		return MB_ENOERR;
	}
	return MB_ENOREG;
}

eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
							  USHORT usNRegs, eMBRegisterMode eMode
)
{
	unsigned int iRegIndex;
	
	/* Check if we have registers mapped at this block. */
	if( (usAddress >= REG_HOLDING_START) &&
		(usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS)
	) {
		switch(eMode) {
		case MB_REG_READ:
			iRegIndex = (int)(usAddress - 1);
			while( usNRegs > 0 ) {
				*pucRegBuffer++ = uiRegHolding[iRegIndex]>>8;
				*pucRegBuffer++ = uiRegHolding[iRegIndex];
				++iRegIndex;
				--usNRegs;
			}
		 return MB_ENOERR;

		/*
		 	Update current register values.
		 	MB_FUNC_WRITE_MULTIPLE_REGISTERS             (16)
		*/
		case MB_REG_WRITE: {
			uint16_t dac = uiRegHolding[4];
			// Запазва текущия работен диапазон на ADC и DAC.
			uint16_t old = 0x0F03 & uiRegHolding[14];
//			uint16_t oldUartControl = 0x000f & uiRegHolding[17];

			iRegIndex = (int)(usAddress - 1);
			while( usNRegs > 0 ) {
				uiRegHolding[iRegIndex]  = (*pucRegBuffer++)<<8;
				uiRegHolding[iRegIndex] |= *pucRegBuffer++;
				++iRegIndex;
				--usNRegs;
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			if( 50 == iRegIndex ) {
				p_factor = uiRegHolding[49];
				pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );
				pid_Reset_Integrator( (pidData_t*)&pidPosData );
			}
			if( 51 == iRegIndex ) {
				i_factor = uiRegHolding[50];
				pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );
				pid_Reset_Integrator( (pidData_t*)&pidPosData );
			}
			if( 52 == iRegIndex ) {
				d_factor = uiRegHolding[51];
				pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );
				pid_Reset_Integrator( (pidData_t*)&pidPosData );
			}
			if( 53 == iRegIndex ) {
				MAX_I_TERM = uiRegHolding[52];
				pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );
				pid_Reset_Integrator( (pidData_t*)&pidPosData );
			}
			if( 54 == iRegIndex ) {
				SCALING_FACTOR = uiRegHolding[53];
				pid_Init( p_factor, i_factor, d_factor, (pidData_t*)&pidPosData );
				pid_Reset_Integrator( (pidData_t*)&pidPosData );
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			if( 57 == iRegIndex ) {
				MoveTo( (int32_t)(uiRegHolding[56])<<16 | uiRegHolding[55] );
			}

			if( 59 == iRegIndex ) {
				SetVelocity( 1024 * ( (int32_t)(uiRegHolding[58])<<16 | uiRegHolding[57] ) );
			}

			if( 61 == iRegIndex ) {
				SetAcceleration( 1024 * ( (int32_t)(uiRegHolding[60])<<16 | uiRegHolding[59] ) );
			}

			if( 63 == iRegIndex ) {
				SetAccAndMaxVelocity( uiRegHolding[61], uiRegHolding[62] );
			}

			if( 65 == iRegIndex ) {
				SetMaxSpeed( (int32_t)(uiRegHolding[64])<<16 | uiRegHolding[63] );
			}

			if( 68 == iRegIndex ) { //???
				SetVelocity( (int32_t)(uiRegHolding[58])<<16 | uiRegHolding[57] );
				SetAcceleration( (int32_t)(uiRegHolding[60])<<16 | uiRegHolding[59] );
				SetAccAndMaxVelocity( uiRegHolding[61], uiRegHolding[62] );
				SetMaxSpeed( (int32_t)(uiRegHolding[64])<<16 | uiRegHolding[63] );
				MoveTo( ((int32_t)(uiRegHolding[56])<<16 | uiRegHolding[55]) );
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////
			if(0x0080 & uiRegHolding[4]) {
				uiRegHolding[4] = (0x007f & dac) | (0xff00 & uiRegHolding[4]);
			}

			if(0x8000 & uiRegHolding[4]) {
				uiRegHolding[4] = (0x7f00 & dac) | (0x00ff & uiRegHolding[4]);
			}

			// Ако не е зададен нов работен диапазон, се възстановява стария:
			if(!(0x0003 & uiRegHolding[14])) uiRegHolding[14] |= (0x0003 & old);
			
			// DAC0 - Запазване на стария работен диапазон:
			if(!(0x0300 & uiRegHolding[14])) uiRegHolding[14] |= (0x0300 & old);
			// DAC1 - Запазване на стария работен диапазон:
			if(!(0x0C00 & uiRegHolding[14])) uiRegHolding[14] |= (0x0C00 & old);
			
			//if(!(0x0003 & uiRegHolding[17])) uiRegHolding[17] |= (0x0003 & oldUartControl);
			//if(!(0x000c & uiRegHolding[17])) uiRegHolding[17] |= (0x000c & oldUartControl);

		}
		 return MB_ENOERR;
		}
	}

	return MB_ENOREG;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ftoa(float f, char *buffer)
{
	int d;
	unsigned char i, str[11];

	if( f < 0 ) {
		f = -f;
		*buffer++ = '-';
	}

	d = f;
	itoa(d, (char*)str, 10);

	i = 0;
	while(str[i]) *buffer++ = str[i++];

	*buffer++ = '.';

	if((int)(((f - d) + 0.001) * 100) < 10) {
		*buffer++ = '0';
	}

	itoa((int)(((f - d) + 0.001) * 100), (char*)str, 10);

	i = 0;
	while(str[i]) *buffer++ = str[i++];

	*buffer = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
