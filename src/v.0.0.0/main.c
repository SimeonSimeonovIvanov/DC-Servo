#include "main.h"

#include <util/delay.h>
#include <string.h>

#define __DIRECTION_AUTO_CHANGE_EVER_MS__ 0

/*
	arrADC[0] - FB Current
	arrADC[1] - FB Speed
	arrADC[2] - SP Current
	arrADC[3] - SP Speed
	arrADC[4] - DC Bus Voltage ???
	arrADC[5] -
*/
volatile int arrADC[4];
volatile unsigned char adcFlag = 0;

/*! \brief Flags for status information
 */
volatile struct GLOBAL_FLAGS {
	//! True when PID control loop should run one time
	uint8_t pidTimer, dummy;
} gFlagsCurrentPid = { 0, 0 }, gFlagsSpeedPid = { 0, 0 };

volatile pidData_t pidSpeedData;
volatile pidData_t pidCurrentData;

volatile int16_t iPWM = 0;
volatile int16_t SpeedSP = 0, SpeedFB = 0, SpeedPidOut = 0;
volatile int16_t CurrentSP = 0, CurrentFB = 0, CurrentPidOut = 0, CurrentLimitSP = 0;

// ----------------------------------- Defines --------------------------------------------
// MB_FUNC_READ_DISCRETE_INPUTS					( 2 )
#define REG_DISC_START							1
#define REG_DISC_SIZE							32

volatile unsigned char ucRegDiscBuf[ REG_DISC_SIZE / 8 ] = { 0, 0 };
uint8_t inPort[ REG_DISC_SIZE ];
// -----------------------------------------------------------------
// MB_FUNC_READ_COILS							( 1 )
// MB_FUNC_WRITE_SINGLE_COIL					( 5 )
// MB_FUNC_WRITE_MULTIPLE_COILS					( 15 )
#define REG_COILS_START							1
#define REG_COILS_SIZE							24

volatile unsigned char ucRegCoilsBuf[ REG_COILS_SIZE / 8 ];
volatile uint8_t outPort[ REG_COILS_SIZE ];
// -----------------------------------------------------------------
// MB_FUNC_READ_INPUT_REGISTER					(  4 )
#define REG_INPUT_START							1
#define REG_INPUT_NREGS							15

volatile uint16_t uiRegInputBuf[ REG_INPUT_NREGS ];
// -----------------------------------------------------------------
// MB_FUNC_READ_HOLDING_REGISTER				( 3 )
// MB_FUNC_WRITE_REGISTER						( 6 )
// MB_FUNC_READWRITE_MULTIPLE_REGISTERS			( 23 )
// MB_FUNC_WRITE_MULTIPLE_REGISTERS				( 16 )
#define REG_HOLDING_START						1
#define REG_HOLDING_NREGS						20

volatile uint16_t uiRegHolding[REG_HOLDING_NREGS];

/* --------------------------------- Static variables ---------------------------------- */

volatile UCHAR ucSlaveID = 10;
volatile uint8_t ucReady = 1, ucOn = 0, ucEMS = 0, ucACmpOverCurrent = 0;
volatile uint8_t mbFB = 0, ucFB = 0;

int main(void)
{
	eMBErrorCode eStatus;
	const UCHAR ucSlaveID[] = { 0xAA, 0xBB, 0xCC };

	DDRB = 1;
	//////////////////////////////////////////////////////////////////////////////
	memset( inPort, 0, sizeof(inPort) );

	registers_init();
	//////////////////////////////////////////////////////////////////////////////
	pidInit();
	//////////////////////////////////////////////////////////////////////////////
	pwm_init();
	pwm_enable();
	TIFR1 |= (1<<ICF1);
	//////////////////////////////////////////////////////////////////////////////
	// Init ADC.
#if _ADC_TIMER1_IC1_TRIGER_ENABLE_
	ADCSRA = (1<<ADEN)  | (0<<ADSC)  | (1<<ADIE)  |
			 (0<<ADPS2) | (0<<ADPS1) | (1<<ADPS0) |
			 (1<<ADATE);

 #ifdef __AVR_ATmega644__
	ADCSRB = (1<<ADTS2) | (1<<ADTS1) | (1<<ADTS0);
  #elif __AVR_ATmega32__
  	SFIOR = (1<<ADTS2) | (1<<ADTS1) | (1<<ADTS0);
  #elif __AVR_ATmega16__
	SFIOR = (1<<ADTS2) | (1<<ADTS1) | (1<<ADTS0);
 #endif
#else
	ADCSRA = (1<<ADEN)  | (1<<ADSC)  | (1<<ADIE)  |
			 (1<<ADPS2) | (0<<ADPS1) | (1<<ADPS0);
#endif

	PORTA = DDRA = 0xF0;
	//////////////////////////////////////////////////////////////////////////////
	// Init Analog Comparator
	//ACSR = (1<<ACIE)  |
	//	   (1<<ACIS1) | (1<<ACIS0); // Comparator Interrupt on Rising Output Edge
	//////////////////////////////////////////////////////////////////////////////

	eStatus = eMBInit( MB_RTU, 0x0A, 0, 115200, MB_PAR_EVEN );
	eStatus = eMBSetSlaveID( 0x34, TRUE, ucSlaveID, 3 );
	sei();
	
	/* Enable the Modbus Protocol Stack. */
	eStatus = eMBEnable();

	while( 1 ) {

		// Run PID calculations once every PID timer timeout
		if( gFlagsCurrentPid.pidTimer ) {
			CurrentFB = arrADC[0];
			
			if( 16 & ucRegCoilsBuf[0] ) {
				CurrentSP = SpeedPidOut;

				if( CurrentSP > CurrentLimitSP ) {
					CurrentSP = CurrentLimitSP;
				} else {
					if( CurrentSP < -CurrentLimitSP ) {
						CurrentSP = -CurrentLimitSP;
					}
				}
			} else {
				CurrentSP = arrADC[2];
				if( mbFB ) {
					CurrentSP = -CurrentSP;
				}
			}

			CurrentPidOut = pid_Controller( CurrentSP, CurrentFB, (pidData_t*)&pidCurrentData );

			iPWM = CurrentPidOut;
			if( ucReady && ucOn ) {
				Set_Input_Current( CurrentPidOut );
			} else {
				Set_Input_Current( 0 );
			}

			if( ++gFlagsSpeedPid.dummy > 10 ) {
				gFlagsSpeedPid.pidTimer = TRUE;
				gFlagsSpeedPid.dummy = 0;
			}

			if( __DIRECTION_AUTO_CHANGE_EVER_MS__ ) {
				static int counter = 0;
			
				SpeedSP = arrADC[3];

				if( ++counter < __DIRECTION_AUTO_CHANGE_EVER_MS__ ) {
					SpeedSP = SpeedSP;
				} else {
					if( ++counter > ( 3 * __DIRECTION_AUTO_CHANGE_EVER_MS__ ) ) {
						counter = 0;
					} else {
						SpeedSP = -SpeedSP;
					}
				}
			}

			gFlagsCurrentPid.pidTimer = FALSE;
			PORTB ^= 1;
		} else {
			if( gFlagsSpeedPid.pidTimer ) {
				
				SpeedFB = arrADC[1];

				if( !__DIRECTION_AUTO_CHANGE_EVER_MS__ ) {
					SpeedSP = arrADC[3];
					if( mbFB ) {
						SpeedSP = -SpeedSP;
					}
				}

				SpeedPidOut = pid_Controller( SpeedSP, SpeedFB, (pidData_t*)&pidSpeedData );

				if( SpeedPidOut >= 0 ) {
				//	ucFB = 0;
				} else {
				//	ucFB = 1;
				}

				CurrentLimitSP = (unsigned int)arrADC[2];

				gFlagsSpeedPid.pidTimer = FALSE;
				PORTB ^= 2;
			}

			cli();
			uiRegInputBuf[0] = CurrentFB;
			uiRegInputBuf[1] = SpeedFB;

			uiRegInputBuf[2] = CurrentSP;
			uiRegInputBuf[3] = SpeedSP;

			uiRegInputBuf[4] = CurrentPidOut;
			uiRegInputBuf[5] = SpeedPidOut;
			uiRegInputBuf[6] = CurrentLimitSP;

			uiRegInputBuf[7] = iPWM;
			uiRegInputBuf[8] = OCR1A;
			uiRegInputBuf[9] = OCR1B;
			sei();

			/*inPort[0] = ucEMS || 0;
			inPort[1] = ucReady;*/
			inPort[0] = ucReady;

			arrToDiscBuff( (unsigned char*)ucRegDiscBuf, inPort, 8 );

			eMBPoll();

			// EMS (Q0):
			if( 1 & ucRegCoilsBuf[0] ) {
				PORTD &= ~( (1<<PD7) | (1<<PD6) | (1<<PD5) | (1<<PD4) );
				pwm_disable();
				PORTD |= ( (1<<PD7) | (1<<PD6) );
				pid_Reset_Integrator( (pidData_t*)&pidCurrentData );
				pid_Reset_Integrator( (pidData_t*)&pidSpeedData );
				ucEMS = 1;
			} else {
				//ucEMS = 0;
			}

			// RESET:
			if( 2 & ucRegCoilsBuf[0] ) {
				pid_Reset_Integrator( (pidData_t *)&pidCurrentData );
				ucRegCoilsBuf[0] &= ~( 2 | 1 );
				ucACmpOverCurrent = 0;
				pwm_enable();
				ucEMS = 0;
			} else {
			}

			if( 4 & ucRegCoilsBuf[0] ) {
				ucOn = 1;
			} else {
				ucOn = 0;
			}

			if( 8 & ucRegCoilsBuf[0] ) {
				mbFB = 1;
			} else {
				mbFB = 0;
			}

			if( ucEMS || ucACmpOverCurrent ) {
				ucReady = 0;
			} else {
				ucReady = 1;
			}
		}
	}

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void pidInit(void)
{
	pid_Init(
		K_P_CURRENT * SCALING_FACTOR,
		K_I_CURRENT * SCALING_FACTOR,
		K_D_CURRENT * SCALING_FACTOR,
		(pidData_t*)&pidCurrentData
	);

	pid_Init(
		K_P_SPEED * SCALING_FACTOR,
		K_I_SPEED * SCALING_FACTOR,
		K_D_SPEED * SCALING_FACTOR,
		(pidData_t*)&pidSpeedData
	);

	uiRegHolding[  0 ] = pidCurrentData.P_Factor;
	uiRegHolding[  1 ] = pidCurrentData.I_Factor;
	uiRegHolding[  2 ] = pidCurrentData.D_Factor;
	uiRegHolding[  3 ] = SCALING_FACTOR;

	uiRegHolding[ 10 ] = pidSpeedData.P_Factor;
	uiRegHolding[ 11 ] = pidSpeedData.I_Factor;
	uiRegHolding[ 12 ] = pidSpeedData.D_Factor;
	uiRegHolding[ 13 ] = SCALING_FACTOR;

	// Set up timer, enable timer/counte 0 overflow interrupt
#ifdef __AVR_ATmega644__
	TCCR0B = (1<<CS01) | (1<<CS00);
	TIMSK0 = (1<<TOIE0);
#elif __AVR_ATmega32__
	TCCR0 = (1<<CS01) | (1<<CS00);
	TIMSK |= (1<<TOIE0);
#elif __AVR_ATmega16__
	TCCR0 = (1<<CS01) | (1<<CS00);
	TIMSK |= (1<<TOIE0);
#endif

	TCNT0 = 5;
}

/*! \brief Set control input to system
 *
 * Set the output from the controller as input
 * to system.
 */
void Set_Input_Current( int16_t inputValue )
{
	int16_t PWM;

	PWM = inputValue;

	ucFB = 0;
	if( PWM < 0 ) {
		PWM = -PWM;
		ucFB = 1;
	}

	if( PWM > 255 ) {
		PWM = 255;
	}

	current_loop_update( PWM, ucFB );

	sei();
}

/*! \brief Timer interrupt to control the sampling interval
 */
ISR( TIMER0_OVF_vect )
{
	TCNT0 = 5;
	gFlagsCurrentPid.pidTimer = TRUE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ISR( INT0_vect )
{
}

ISR( INT1_vect )
{
}
//////////////////////////////////////////////////////////////////////////////////
ISR( ADC_vect )
{
	volatile static unsigned char channel = 0;
/*#if !_ADC_TIMER1_IC1_TRIGER_ENABLE_
	static unsigned char n = 0;

	if( 2 > ++n ) {
		ADCSRA |= (1<<ADSC);
		return;
	} else {
		n = 0;
	}
#endif*/

	arrADC[ channel ] = ADC;

	if( 1 == channel ) {
		if( PIND & (1<<PD3) ) {
			arrADC[ channel ] = -arrADC[ channel ];
		}
	}

	if( ++channel >= ( sizeof(arrADC) / sizeof(*arrADC) ) ) {
		channel = 0;
		adcFlag = 1;
	}

	ADMUX = channel;

#if _ADC_TIMER1_IC1_TRIGER_ENABLE_
	  else {
		ADCSRA |= (1<<ADSC);
	}
	TIFR1 |= (1<<ICF1);
#else
	ADCSRA |= (1<<ADSC);
#endif
}

#ifdef __AVR_ATmega644__
ISR( ANALOG_COMP_vect )
#elif __AVR_ATmega32__
ISR( ANA_COMP_vect )
#elif __AVR_ATmega16__
ISR( ANA_COMP_vect )
#endif
{
	PORTD &= ~( (1<<PD7) | (1<<PD6) | (1<<PD5) | (1<<PD4) );
	ucACmpOverCurrent = 1;
	pwm_disable();
}
///////////////////////////////////////////////////////////////////////////////////////////
eMBErrorCode eMBRegDiscreteCB
(
	UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete
)
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
			xMBUtilGetBits( (unsigned char*)ucRegDiscBuf, usBitOffset,
                            (unsigned char)(iNDiscrete>8? 8:iNDiscrete)
			);
			iNDiscrete -= 8;
			usBitOffset += 8;
		}
		return MB_ENOERR;
	}

	return eStatus;
}

eMBErrorCode eMBRegCoilsCB
(
	UCHAR * pucRegBuffer, USHORT usAddress,
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
				xMBUtilGetBits( (unsigned char*)ucRegCoilsBuf, usBitOffset,
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
				xMBUtilSetBits( (unsigned char*)ucRegCoilsBuf, usBitOffset,
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

eMBErrorCode eMBRegInputCB
(
	UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs
)
{
	unsigned int iRegIndex;
 
	// MB_FUNC_READ_INPUT_REGISTER           (  4 )
	if( (usAddress >= REG_INPUT_START) &&
		(usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS)
	) {
		iRegIndex = (int)(usAddress - REG_INPUT_START);
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

eMBErrorCode eMBRegHoldingCB
(
	UCHAR * pucRegBuffer, USHORT usAddress,
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
			iRegIndex = (int)(usAddress - 1);
			while( usNRegs > 0 ) {
				uiRegHolding[iRegIndex]  = (*pucRegBuffer++)<<8;
				uiRegHolding[iRegIndex] |= *pucRegBuffer++;
				++iRegIndex;
				--usNRegs;
			}

			switch( iRegIndex ) {
			case 1: {
				pid_Init(
					uiRegHolding[ 0 ],
					pidCurrentData.I_Factor,
					pidCurrentData.D_Factor,
					(pidData_t*)&pidCurrentData
				);
				pid_Reset_Integrator( (pidData_t*)&pidCurrentData );
			}
			 break;
			case 2: {
				pid_Init(
					pidCurrentData.P_Factor,
					uiRegHolding[ 1 ],
					pidCurrentData.D_Factor,
					(pidData_t*)&pidCurrentData
				);
				pid_Reset_Integrator( (pidData_t*)&pidCurrentData );
			}
			 break;
			case 3: {
				pid_Init(
					pidCurrentData.P_Factor,
					pidCurrentData.I_Factor,
					uiRegHolding[ 2 ],
					(pidData_t*)&pidCurrentData
				);
				pid_Reset_Integrator( (pidData_t*)&pidCurrentData );
			}
			 break;

			case 11: {
				pid_Init(
					uiRegHolding[ 10 ],
					pidSpeedData.I_Factor,
					pidSpeedData.D_Factor,
					(pidData_t*)&pidSpeedData
				);
				pid_Reset_Integrator( (pidData_t*)&pidSpeedData );
			}
			 break;
			case 12: {
				pid_Init(
					pidSpeedData.P_Factor,
					uiRegHolding[ 11 ],
					pidSpeedData.D_Factor,
					(pidData_t*)&pidSpeedData
				);
				pid_Reset_Integrator( (pidData_t*)&pidSpeedData );
			}
			 break;
			case 13: {
				pid_Init(
					pidSpeedData.P_Factor,
					pidSpeedData.I_Factor,
					uiRegHolding[ 12 ],
					(pidData_t*)&pidSpeedData
				);
				pid_Reset_Integrator( (pidData_t*)&pidSpeedData );
			}
			 break;
			}
		}
		 return MB_ENOERR;
		}
	}

	return MB_ENOREG;
}
///////////////////////////////////////////////////////////////////////////////////////////
void arrToDiscBuff( unsigned char *ucRegDiscBuf, uint8_t *inPort, uint8_t n )
{
	int i, j;

	for( i = 0; i < n; i++ ) {
		j = i / 8;

		if( inPort[ i ] ) {
			ucRegDiscBuf[ j ] |=  ( 1<<( i - ( 8 * j ) ) );
		} else {
			ucRegDiscBuf[ j ] &= ~( 1<<( i - ( 8 * j ) ) );
		}
	}
}
