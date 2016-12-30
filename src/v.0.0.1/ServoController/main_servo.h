#ifndef __MAIN_SERVO_H__
#define __MAIN_SERVO_H__

#include "../main.h"
#include "motion.h"
#include "encoder.h"

#define ENABLE_MOTOR			1		// <1><1>
#define DISABLE_MOTOR			2		// <2><0>
#define MOVE_TO_CMD				3		// <3><Byte0><Byte1><Byte2><Byte3>	- absolute position in steps
#define MOVE_CMD				4		// <4><Byte0><Byte1><Byte2><Byte3>	- relative move in steps
#define SET_SPEED_CMD			5		// <5><Byte0><Byte1><Byte2><Byte3>	- move at a set speed in steps/sec

#define SET_SPEED_PERIOD_CMD	10		// <10><Byte0><Byte1><Byte2><Byte3>	- set the time a SET_SPEED_CMD will continue for (ms)
#define SET_MAX_SPEED_CMD		11		// <11><Byte0><Byte1><Byte2><Byte3>	- steps/second
#define SET_ACCEL_CMD			12		// <12><Byte0><Byte1><Byte2><Byte3>	- steps/second/second

#define GET_SPEED_CMD			20		// <20>	?
#define GET_POSITION_CMD		21		// <21>	?
#define QUERY_STATE				22		// <22>	returns <status><pos0><pos1><pos2>

#define SET_ADDRESS_CMD			40		// <40><0x42><0x24><0x00><Address>
#define SET_PID_P_CMD			41		// <41><LSB><MSB>
#define SET_PID_I_CMD			42		// <42><LSB><MSB>
#define SET_PID_D_CMD			43		// <43><LSB><MSB>
#define SET_PID_SCALE_CMD		44		// <44><LSB><MSB>
#define SET_PID_ERROR_CMD		45		// <45><LSB><MSB>		- fault if PID error gets above this.  0 to disable.

#define GET_ADDRESS_CMD			60		// <60><0x42><0x24><0x00><Address>
#define GET_PID_P_CMD			61		// <61><LSB><MSB>
#define GET_PID_I_CMD			62		// <62><LSB><MSB>
#define GET_PID_D_CMD			63		// <63><LSB><MSB>
#define GET_PID_SCALE_CMD		64		// <64><LSB><MSB>
#define GET_PID_ERROR_CMD		65		// <65><LSB><MSB>		- fault if PID error gets above this.  0 to disable.

void servoInit( void );
int16_t servoLoop( void );
void servoInitForFirstRun( void );

#endif
