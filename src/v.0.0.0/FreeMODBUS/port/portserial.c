/*
 * FreeModbus Libary: ATMega168 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *   - Initial version and ATmega168 support
 * Modfications Copyright (C) 2006 Tran Minh Hoang:
 *   - ATmega8, ATmega16, ATmega32 support
 *   - RS485 support for DS75176
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.6 2006/09/17 16:45:53 wolti Exp $
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

#define UART_BAUD_CALC(UART_BAUD_RATE,F_OSC) \
	( ( ( F_OSC ) / ( UART_BAUD_RATE ) / 16UL ) - 1 )

extern uint8_t uiModbusTimeOutCounter;

void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
#ifdef RTS_ENABLE
	UCSRB |= _BV( TXEN ) | _BV(TXCIE);
#else
	UCSRB |= _BV( TXEN );
#endif

	if( xRxEnable ) {
		UCSRB |= _BV( RXEN ) | _BV( RXCIE );
	} else {
		UCSRB &= ~( _BV( RXEN ) | _BV( RXCIE ) );
	}

	if( xTxEnable ) {
		UCSRB |= _BV( TXEN ) | _BV( UDRE );
#ifdef RTS_ENABLE
		RTS_HIGH;
#endif
	} else {
		UCSRB &= ~( _BV( UDRE ) );
	}
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	UCHAR ucUCSRC = 0;
	uint16_t uiBaudRate = 0;

	// F_CPU: 16.00 MHz
	switch( ulBaudRate ) {
	case 2400: uiBaudRate = 416; break;
	case 4800: uiBaudRate = 207; break;
	case 9600: uiBaudRate = 103; break;
	case 14400: uiBaudRate = 68; break;
	case 19200: uiBaudRate = 51; break;
	case 28800: uiBaudRate = 34; break;
	case 38400: uiBaudRate = 25; break;
	case 57600: uiBaudRate = 16; break;
	case 76800: uiBaudRate = 12; break;
	case 115200: uiBaudRate = 8; break;
	case 230400: uiBaudRate = 3; break;
	case 250000: uiBaudRate = 3; break;
	case 500000: uiBaudRate = 1; break;
	case 1000000: uiBaudRate = 0; break;
	default: uiBaudRate = UART_BAUD_CALC( ulBaudRate, F_CPU );
	}

	UBRRH = uiBaudRate>>8;
	UBRRL = uiBaudRate;

	switch ( eParity ) {
	case MB_PAR_EVEN:
		ucUCSRC |= _BV( UPM_1 );
	 break;
	case MB_PAR_ODD:
		ucUCSRC |= _BV( UPM_1 ) | _BV( UPM_0 );
	 break;
	case MB_PAR_NONE:
	 break;
	}

	switch ( ucDataBits ) {
	case 8:
		ucUCSRC |= _BV( USBS_0 ) | _BV( UCSZ_1 ) | _BV( UCSZ_0 );
	 break;
	case 7:
		ucUCSRC |= _BV( UCSZ_1 );
	 break;
	}

#if defined (__AVR_ATmega168__)
	UCSRC |= ucUCSRC;
#elif defined (__AVR_ATmega169__)
	UCSRC |= ucUCSRC;
#elif defined (__AVR_ATmega8__)
	UCSRC = _BV( URSEL ) | ucUCSRC;
#elif defined (__AVR_ATmega16__)
	UCSRC = _BV( URSEL ) | ucUCSRC;
#elif defined (__AVR_ATmega32__)
	UCSRC = _BV( URSEL ) | ucUCSRC;
#elif defined (__AVR_ATmega128__)
	UCSRC = ucUCSRC;
#endif

	vMBPortSerialEnable( FALSE, FALSE );

#ifdef RTS_ENABLE
	RTS_INIT;
#endif
	return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
#ifdef RTS_ENABLE
	RTS_HIGH;
#endif
	UDR = ucByte;
	return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{

	*pucByte = UDR;
	return TRUE;
}

ISR( SIG_UART_TRANS )
{
#ifdef RTS_ENABLE
	RTS_LOW;
#endif
}

ISR( SIG_USART_RECV )
{
	pxMBFrameCBByteReceived(  );

//	uiModbusTimeOutCounter = 200;
}

ISR( SIG_USART_DATA )
{
	pxMBFrameCBTransmitterEmpty(  );
}
