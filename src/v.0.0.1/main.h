#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <stdbool.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "app/clock_sync.h"
#include "app/dhcp_client.h"
#include "app/httpd.h"
#include "arch/spi.h"
#include "arch/uart.h"
#include "net/arp.h"
#include "net/ethernet.h"
#include "net/hal.h"
#include "net/icmp.h"
#include "net/ip.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "sd/fat16.h"
#include "sd/sd.h"
#include "sd/sd_raw.h"
#include "sys/clock.h"
#include "sys/timer.h"

#include "adc/adc.h"
#include "dac/dac.h"

#include "mcp23sxx/mcp23sxx.h"
#include "mcp23sxx/mcp23s08.h"
#include "mcp23sxx/mcp23s17.h"

#include "net/enc424j600/enc424j600.h"

#include "ServoController/main_servo.h"

#include "pid/pid_atmel.h"

#define CPU_CS0_bp						(PB7)
#define CPU_CS0_bm						(1<<CPU_CS0_bp)
#define CPU_CS1_bp						(PB6)
#define CPU_CS1_bm						(1<<CPU_CS1_bp)
#define CPU_CS2_bp						(PB5)
#define CPU_CS2_bm						(1<<CPU_CS2_bp)
#define CPU_CS3_bp						(PB4)
#define CPU_CS3_bm						(1<<CPU_CS3_bp)
#define CPU_CS_NET_bp					(PB0)
#define CPU_CS_NET_bm					(1<<CPU_CS_NET_bp)

#define CPU_CS_RS485_ADDRESS_bp			(PD7)
#define CPU_CS_RS485_ADDRESS_bm			(1<<CPU_CS_RS485_ADDRESS_bp)
#define RS485_DE_bp						(PD6)
#define RS485_DE_bm						(1<<RS485_DE_bp)
#define CPU_CS_AO_bp					(PD5)
#define CPU_CS_AO_bm					(1<<CPU_CS_AO_bp)
#define ID6_IC1_bp						(PD4)
#define ID6_IC1_bm						(1<<ID6_IC1_bp)
#define RS485_TX_bp						(PD3)
#define RS485_TX_bm						(1<<RS485_TX_bp)
#define RS485_RX_bp						(PD2)
#define RS485_RX_bm						(1<<RS485_RX_bp)
#define CPU_SDA_bp						(PD1)
#define CPU_SDA_bm						(CPU_SDA_bp)
#define CPU_SCK_bp						(PD0)
#define CPU_SCK_bm						(1<<CPU_SCK_bp)

#define CPU_NET_INT_bp					(PE7)
#define CPU_NET_INT_bm					(1<<CPU_NET_INT_bp)
#define ID7_INT6_bp						(PE6)
#define ID7_INT6_bm						(1<<ID7_INT6_bp)
#define ID8_INT5_bp						(PE5)
#define ID8_INT5_bm						(1<<ID8_INT5_bp)
#define ID9_INT4_bp						(PE4)
#define ID9_INT4_bm						(1<<ID9_INT4_bp)
#define CPU_CS_DO_bp					(PE3)
#define CPU_CS_DO_bm					(1<<CPU_CS_DO_bp)
#define CPU_CS_DI_bp					(PE2)
#define CPU_CS_DI_bm					(1<<CPU_CS_DI_bp)
#define CPU_TXD_bp						(PE1)
#define CPU_TXD_bm						(1<<CPU_TXD_bp)
#define CPU_RXD_bp						(PE0)
#define CPU_RXD_bm						(1<<CPU_RXD_bp)

#define CPU_IO_RESET_bp					(PF1)
#define CPU_IO_RESET_bm					(1<<CPU_IO_RESET_bp)
#define CPU_CS_AI_bp					(PF0)
#define CPU_CS_AI_bm					(1<<CPU_CS_AI_bp)

#define enable_io_reset()				(PORTF &= ~CPU_IO_RESET_bm)
#define disable_io_reset()				(PORTF |= CPU_IO_RESET_bm)

#define select_analog_in()				(PORTF &= ~CPU_CS_AI_bm)
#define unselect_analog_in()			(PORTF |= CPU_CS_AI_bm)
#define select_analog_out()				(PORTD &= ~CPU_CS_AO_bm)
#define unselect_analog_out()			(PORTD |= CPU_CS_AO_bm)

#define select_digital_in()				(PORTE &= ~CPU_CS_DI_bm)
#define unselect_digital_in()			(PORTE |= CPU_CS_DI_bm)
#define select_digital_out()			(PORTE &= ~CPU_CS_DO_bm)
#define unselect_digital_out()			(PORTE |= CPU_CS_DO_bm)

#define enable_rs485_transmit()			(PORTD |= RS485_DE_bm)
#define disable_rs485_transmit()		(PORTD &= ~RS485_DE_bm)
#define select_address_switch()			(PORTD &= ~CPU_CS_RS485_ADDRESS_bm)
#define unselect_address_switch()		(PORTD |= CPU_CS_RS485_ADDRESS_bm)

/////////////////////////////////////////////////////
void initAddressSwitch(void);
char readAddressSwitch(void);
/////////////////////////////////////////////////////
void initDigitalInput(uint8_t in[16]);
void readDigitalInput(uint8_t in[16]);
/////////////////////////////////////////////////////
void initDigitalOutput(uint8_t out[12]);
void writeDigitalOutput(uint8_t out[12]);
/////////////////////////////////////////////////////

void ftoa(float f, char *buffer);

#endif
