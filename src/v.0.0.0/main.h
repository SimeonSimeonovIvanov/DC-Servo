#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>

//#include "openservo.h"
#include "config.h"
//#include "adc.h"
#include "eeprom.h"
//#include "motion.h"
#include "pid.h"
//#include "power.h"
#include "pwm.h"
/*#include "seek.h"
#include "pulsectl.h"
#include "timer.h"
#include "twi.h"
#include "watchdog.h"*/
#include "registers.h"
//#include "swi2c.h"
//#include "enc.h"

#include "current_loop.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbutils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// User Defined:
/*! \brief P, I and D parameter values
 *
 * The K_P, K_I and K_D values (P, I and D gains)
 * need to be modified to adapt to the application at hand
 */
#define K_P_CURRENT		0.20
#define K_I_CURRENT		0.00
#define K_D_CURRENT		0.00

#define K_P_SPEED		1.00
#define K_I_SPEED		0.00
#define K_D_SPEED		0.00
///////////////////////////////////////////////////////////////////////////////////////////////////

#define _ADC_TIMER1_IC1_TRIGER_ENABLE_		( 0 )

///////////////////////////////////////////////////////////////////////////////////////////////////

void pidInit(void);
int16_t Get_Reference(void);
int16_t Get_Measurement(void);
void Set_Input_Current( int16_t inputValue );

eMBErrorCode eMBRegDiscreteCB
(
	UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete
);
eMBErrorCode eMBRegCoilsCB
(
	UCHAR * pucRegBuffer, USHORT usAddress,
	USHORT usNCoils, eMBRegisterMode eMode
);
eMBErrorCode eMBRegInputCB
(
	UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs
);
eMBErrorCode eMBRegHoldingCB
(
	UCHAR * pucRegBuffer, USHORT usAddress,
	USHORT usNRegs, eMBRegisterMode eMode
);

void arrToDiscBuff( unsigned char *ucRegDiscBuf, uint8_t *inPort, uint8_t n );
