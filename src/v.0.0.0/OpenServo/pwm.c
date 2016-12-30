/*
	Copyright (c) 2006 Michael P. Thompson <mpthompson@gmail.com>

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	$Id$
*/

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "openservo.h"
#include "config.h"
#include "pwm.h"
#include "registers.h"

//
// ATmega16 ???
// =========
//
// PWM output to the servo motor utilizes Timer/Counter1 in 8-bit mode.
// Output to the motor is assigned as follows:
//
//  PD5/OC1A - PWM_A	- PWM pulse output direction A
//  PD4/OC1B - PWM_B	- PWM pulse output direction B
//  PD7	  - EN_A		- PWM enable direction A
//  PD6	  - EN_B		- PWM enable direction B
//  Pxx	  - SMPLn_B		- BEMF sample enable B
//  Pxx	  - SMPLn_A		- BEMF sample enable A
//

// Determine the top value for timer/counter1 from the frequency divider.
#define PWM_TOP_VALUE( div )			( ( (uint16_t)div<<4 ) - 1 );

// Determines the compare value associated with the duty cycle for timer/counter1.
#define PWM_OCRN_VALUE( div, pwm )		(uint16_t)( ((uint32_t) pwm * (((uint32_t) div << 4) - 1)) / 255 )

// Flags that indicate PWM output in A and B direction.
static uint8_t pwm_a;
static uint8_t pwm_b;

// Pwm frequency divider value.
static uint16_t pwm_div;

//
// The delay_loop function is used to provide a delay. The purpose of the delay is to
// allow changes asserted at the AVRs I/O pins to take effect in the H-bridge (for example
// turning on or off one of the MOSFETs). This is to prevent improper states from occurring
// in the H-bridge and leading to brownouts or "crowbaring" of the supply. This was more
// of a problem, prior to the introduction of the delay, with the faster processor clock
// rate that was introduced with the V3 boards (20MHz) than it was with the older8Hhz V2.1
// boards- there was still a problem with the old code, with that board, it was just less
// pronounced.
//
// NOTE: Lower impedance motors may increase the lag of the H-bridge and thus require longer
//	   delays.
//
#define DELAYLOOP 70 // TODO: This needs to be adjusted to account for the clock rate.
					 //	   This value of 20 gives a 5 microsecond delay and was chosen
					 //	   by experiment with an "MG995".
inline static void delay_loop(int n)
{
	uint8_t i;
	for( i = 0; i < n; i++ ) {
		asm("nop\n");
	}
}

void pwm_dir_a( uint8_t pwm_duty )
// Send PWM signal for rotation with the indicated pwm ratio (0 - 255).
// This function is meant to be called only by pwm_update.
{
	// Determine the duty cycle value for the timer.
	uint16_t duty_cycle = PWM_OCRN_VALUE( pwm_div, pwm_duty );

	cli();
	// Do we need to reconfigure PWM output for direction A?
	if( !pwm_a ) { // Yes...
		// Set SMPLn_B and SMPLn_A to high.
		//PORTx |= ((1<<Pxx) | (1<<Pxx));

		// Set EN_B to low.
		PORTD &= ~(1<<PD7);

		// Disable PWM_A and PWM_B output.
		// NOTE: Actually PWM_A should already be disabled...
		TCCR1A = 0;

		// Make sure PWM_A and PWM_B are low.
		PORTD &= ~( (1<<PD5) | (1<<PD4) );

		// Give the H-bridge time to respond to the above, failure to do so or to wait long
		// enough will result in brownouts as the power is "crowbarred" to varying extents.
		// The delay required is also dependant on factors which may affect the speed with
		// which the MOSFETs can respond, such as the impedance of the motor, the supply
		// voltage, etc.
		//
		// Experiments (with an "MG995") have shown that 5microseconds should be sufficient
		// for most purposes.
		//
		delay_loop( DELAYLOOP );

		// Enable PWM_A (PD5/OC1A)  output.
		TCCR1A = (1<<COM1A1);

		// Set EN_A to high.
		PORTD |= (1<<PD6);

		// NOTE: The PWM driven state of the H-bridge should not be switched to b-mode or braking
		// without a suffient delay.

		// Reset the B direction flag.
		pwm_b = 0;
	}

	// Update the A direction flag.  A non-zero value keeps us from
	// recofiguring the PWM output A when it is already configured.
	pwm_a = pwm_duty;

	// Update the PWM duty cycle.
	OCR1A = duty_cycle;
	OCR1B = 0;

	// Restore interrupts.
	sei();
}

void pwm_dir_b( uint8_t pwm_duty )
// Send PWM signal for rotation with the indicated pwm ratio (0 - 255).
// This function is meant to be called only by pwm_update.
{
	// Determine the duty cycle value for the timer.
	uint16_t duty_cycle = PWM_OCRN_VALUE( pwm_div, pwm_duty );

	cli();
	// Do we need to reconfigure PWM output for direction B?
	if( !pwm_b ) { // Yes...
		// Set SMPLn_B and SMPLn_A to high.
//		PORTx |= ((1<<Pxx) | (1<<Pxx));

		// Set EN_A to low.
		PORTD &= ~(1<<PD6);

		// Disable PWM_A and PWM_B output.
		// NOTE: Actually PWM_B should already be disabled...
		TCCR1A = 0;

		// Make sure PWM_A and PWM_B are low.
		PORTD &= ~( (1<<PD5) | (1<<PD4) );

		// Give the H-bridge time to respond to the above, failure to do so or to wait long
		// enough will result in brownouts as the power is "crowbarred" to varying extents.
		// The delay required is also dependant on factors which may affect the speed with
		// which the MOSFETs can respond, such as the impedance of the motor, the supply
		// voltage, etc.
		//
		// Experiments (with an "MG995") have shown that 5microseconds should be sufficient
		// for most purposes.
		//
		delay_loop( DELAYLOOP );

		// Enable PWM_B output.
		TCCR1A = (1<<COM1B1);

		// Set EN_B to high.
		PORTD |= (1<<PD7);

		// NOTE: The PWM driven state of the H-bridge should not be switched to a-mode or braking
		// without a suffient delay.

		// Reset the A direction flag.
		pwm_a = 0;
	}

	// Update the B direction flag.  A non-zero value keeps us from
	// recofiguring the PWM output B when it is already configured.
	pwm_b = pwm_duty;

	// Update the PWM duty cycle.
	OCR1A = 0;
	OCR1B = duty_cycle;

	// Restore interrupts.
	sei();
}

void pwm_stop(void)
// Stop all PWM signals to the motor.
{
	// Disable interrupts.
	cli();

	TCCR1A = 0;
	PORTD &= ~( (1<<PD7) | (1<<PD6) | (1<<PD5) | (1<<PD4) );
	OCR1A = 0;
	OCR1B = 0;

	// Make sure that SMPLn_B and SMPLn_A are held high.
//	PORTx |= ( (1<<Pxx) | (1<<Pxx) );

	// Reset the A and B direction flags.
	pwm_a = 0;
	pwm_b = 0;

	// Restore interrupts.
	sei();
}

void pwm_init(void)
// Initialize the PWM module for controlling a DC motor.
{
	// Initialize the pwm frequency divider value.
	pwm_div = registers_read_word( REG_PWM_FREQ_DIVIDER_HI, REG_PWM_FREQ_DIVIDER_LO );

	// Set SMPLn_B and SMPLn_A to high.
//	PORTx |= ( (1<<Pxx) | (1<<Pxx) );
//	DDRx |= ( (1<<PDx) | (1<<PDx) );

	// Set EN_A and EN_B to low.
	PORTD &= ~( (1<<PD7) | (1<<PD6) );
	DDRD |= ( (1<<PD7) | (1<<PD6) );

	// Set PWM_A and PWM_B are low.
	PORTD &= ~( (1<<PD5) | (1<<PD4) );
	// Enable OC1A and OC1B as outputs.
	DDRD |= ( (1<<PD5) | (1<<PD4) );

	// Reset the timer1 configuration.
	TCNT1 = 0;
	TCCR1A = 0;
	TCCR1B = 0;

	// Set timer top value.
	ICR1 = PWM_TOP_VALUE( pwm_div );

	// Set the PWM duty cycle to zero.
	OCR1A = 0;
	OCR1B = 0;

	// Configure timer 1 for PWM, Phase and Frequency Correct operation, but leave outputs disabled.
	TCCR1A = (0<<COM1A1) | (0<<COM1A0) |			// Disable OC1A output.
			 (0<<COM1B1) | (0<<COM1B0) |			// Disable OC1B output.
			 (0<<WGM11)  | (0<<WGM10);				// PWM, Phase and Frequency Correct, TOP = ICR1
	TCCR1B = (0<<ICNC1)  | (0<<ICES1)  |			// Input on ICP1 disabled.
			 (1<<WGM13)  | (0<<WGM12)  |			// PWM, Phase and Frequency Correct, TOP = ICR1
			 (0<<CS12)   | (0<<CS11)   | (1<<CS10);	// No prescaling.
}

void pwm_registers_defaults(void)
// Initialize the PWM algorithm related register values.  This is done
// here to keep the PWM related code in a single file.
{
	// PWM divider is a value between 1 and 1024.  This divides the fundamental
	// PWM frequency (500 kHz for 8MHz clock, 1250 kHz for 20MHz clock) by a
	// constant value to produce a PWM frequency suitable to drive a motor.  A
	// small motor with low inductance and impedance such as those found in an
	// RC servo will my typically use a divider value between 16 and 64.  A larger
	// motor with higher inductance and impedance may require a greater divider.
	registers_write_word( REG_PWM_FREQ_DIVIDER_HI, REG_PWM_FREQ_DIVIDER_LO, DEFAULT_PWM_FREQ_DIVIDER );
}
