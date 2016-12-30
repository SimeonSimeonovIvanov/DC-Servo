/*
 * FreeModbus Libary: ATMega168 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: porttimer.c,v 1.4 2006/09/03 11:53:10 wolti Exp $
 */

/* ----------------------- AVR includes -------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_TIMER_PRESCALER		( 1024UL )
#define MB_TIMER_TICKS			( F_CPU / MB_TIMER_PRESCALER )
#define MB_50US_TICKS			( 20000UL )
//#define MB_TIMER_TICKS			(F_CPU / MB_TIMER_PRESCALER)
//#define MB_50US_TICKS				(MB_TIMER_TICKS / (MB_TIMER_TICKS / 20000))

/* ----------------------- Static variables ---------------------------------*/
//static USHORT   usTimerOCRADelta;
//static USHORT   usTimerOCRBDelta;

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
    /* Calculate overflow counter an OCR values for Timer1. */
    /*usTimerOCRADelta =
        ( MB_TIMER_TICKS * usTim1Timerout50us ) / ( MB_50US_TICKS );

    TCCR1A = 0x00;
    TCCR1B = 0x00;
    TCCR1C = 0x00;*/

#ifdef __AVR_ATmega644__
	TCCR2B = 0;
	TIMSK2 = 0;
#elif __AVR_ATmega32__
	TCCR2 = 0;
	TIMSK &= ~(1<<TOIE2);
#elif __AVR_ATmega16__
	TCCR2 = 0;
	TIMSK &= ~(1<<TOIE2);
#endif
	TCNT2 = 5;

    vMBPortTimersDisable(  );

    return TRUE;
}

inline void
vMBPortTimersEnable(  )
{
	/*TCNT1 = 0x0000;
    if( usTimerOCRADelta > 0 )
    {
        TIMSK1 |= _BV( OCIE1A );
        OCR1A = usTimerOCRADelta;
    }
	TCCR1B |= _BV( CS12 ) | _BV( CS10 );*/

#ifdef __AVR_ATmega644__
	TCCR2B = (1<<CS21) | (1<<CS20);
	TIMSK2 = (1<<TOIE2);
#elif __AVR_ATmega32__
	TCCR2 = (1<<CS21) | (1<<CS20);
	TIMSK |= (1<<TOIE2);
#elif __AVR_ATmega16__
	TCCR2 = (1<<CS21) | (1<<CS20);
	TIMSK |= (1<<TOIE2);
#endif
	TCNT2 = 5;
}

inline void
vMBPortTimersDisable(  )
{
    /* Disable the timer. */
//	TCCR1B &= ~( _BV( CS12 ) | _BV( CS10 ) );
    /* Disable the output compare interrupts for channel A/B. */
//	TIMSK1 &= ~( _BV( OCIE1A ) );
    /* Clear output compare flags for channel A/B. */
//	TIFR1 |= _BV( OCF1A ) ;

#ifdef __AVR_ATmega644__
	TCCR2B &= ~(1<<CS21) | (1<<CS20);
	TIMSK2 &= ~(1<<TOIE2);
	TIFR2 |= _BV( OCF2A );
#elif __AVR_ATmega32__
	TCCR2 &= ~( (1<<CS21) | (1<<CS20) );
	TIMSK &= ~(1<<TOIE2);
	TIFR |= _BV( OCF2 );
#elif __AVR_ATmega16__
	TCCR2 &= ~( (1<<CS21) | (1<<CS20) );
	TIMSK &= ~(1<<TOIE2);
	TIFR |= _BV( OCF2 );
#endif
	TCNT2 = 0;
}

//ISR(SIG_OUTPUT_COMPARE1A)
//{
//   ( void )pxMBPortCBTimerExpired(  );
//}

ISR( TIMER2_OVF_vect )
{
	TCNT2 = 5;
	( void )pxMBPortCBTimerExpired(  );
}
