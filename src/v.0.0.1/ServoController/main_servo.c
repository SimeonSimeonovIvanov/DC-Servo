// MotorDriver2, firmware for the I2C Servo Motor Controller
// Copyright (C) 2008, Frank Tkalcevic, www.franksworkshop.com

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "main_servo.h"

volatile int32_t nEncoderPosition;
volatile uint8_t bDoPID;

int16_t servoLoop( void )
{
	static volatile int out = 0;

	//char TWI_buf[10] = { 0 };

	if( 0 ){//bHasMessage ) {
//		bHasMessage = 0;
		//ProcessMessage();
//		TWI_Slave_Start();	// listen for next message
	} else {
//		if( !bTWIActive ) {
//			TWI_Slave_Start();	// listen for next message
//		}
	}


	if( bDoPID ) {
		cli();
		bDoPID = 0;
		nEncoderPosition;
		sei();

		//out = DoPositionPIDLoop();
		MotionUpdate();
	}

	return out;
}

void servoProcessMessage( char *TWI_buf )
{
	switch( TWI_buf[0] ) {
	case ENABLE_MOTOR:			// <1><1>
		if( 1 == TWI_buf[1] ) {
		//	EnableHBridge( true );
		}
	 break;

	case DISABLE_MOTOR:			// <2><0>
		if( 0 == TWI_buf[1] ) {
		//	EnableHBridge( false );
		}
	 break;

	case MOVE_TO_CMD:			// <3><Byte0><Byte1><Byte2><Byte3>	- absolute position in steps
		MoveTo( *(int32_t *)(TWI_buf + 1) );
	 break;

	case MOVE_CMD:				// <4><Byte0><Byte1><Byte2><Byte3>	- relative move in steps
		Move( *(int32_t *)(TWI_buf + 1) );
	 break;

	case SET_SPEED_CMD:			// <5><Byte0><Byte1><Byte2><Byte3>	- move at a set speed in steps/sec
		SetVelocity( *(int32_t *)(TWI_buf + 1) );
	 break;

	case SET_SPEED_PERIOD_CMD:	// <10><Byte0><Byte1><Byte2><Byte3>	- set the time a SET_SPEED_CMD will continue for (ms)
		SetSpeedPeriod( *(int32_t *)(TWI_buf + 1) );
	 break;

	case SET_MAX_SPEED_CMD:		// <0x02><Byte0><Byte1><Byte2><Byte3>	- steps/second
		SetMaxSpeed( *(int32_t *)(TWI_buf + 1) );
	 break;

	case SET_ACCEL_CMD:			// <12><Byte0><Byte1><Byte2><Byte3>	- steps/second/second
		SetAcceleration( *(int32_t *)(TWI_buf + 1) );
	 break;

	case GET_SPEED_CMD:			// <20>	?
	 break;

	case GET_POSITION_CMD:		// <21>	?
	 break;

	case QUERY_STATE:			// <22>	returns <status><pos0><pos1><pos2>
//		HandleQueryState();
	 break;

	case SET_ADDRESS_CMD:		// <40><0x42><0x24><0x00><Address>
		if( TWI_buf[1] == 0x42 && TWI_buf[2] == 0x24 && !TWI_buf[3] ) {
//			SetTWIAddress( TWI_buf[4] );
		}
	break;

	case SET_PID_P_CMD:			// <41><LSB><MSB>
//		SetPID_P( *(int16_t *)(TWI_buf + 1) );
	 break;

	case SET_PID_I_CMD:			// <42><LSB><MSB>
//		SetPID_I( *(int16_t *)(TWI_buf + 1) );
	 break;

	case SET_PID_D_CMD:			// <43><LSB><MSB>
//		SetPID_D( *(int16_t *)(TWI_buf + 1) );
	 break;

	case SET_PID_SCALE_CMD:		// <44><LSB><MSB>
//		SetPID_Scale( *(int16_t *)(TWI_buf + 1) );
	 break;

	case SET_PID_ERROR_CMD:		// <45><LSB><MSB>		- fault if PID error gets above this.  0 to disable.
//		SetPID_Error( *(int16_t *)(TWI_buf + 1) );
	 break;

	case GET_ADDRESS_CMD:		// <60>
//		ReplyByte( GetTWIAddress() );
	 break;

	case GET_PID_P_CMD:			// <61>
//		ReplyInt( GetPID_P() );
	 break;

	case GET_PID_I_CMD:			// <62>
//		ReplyInt( GetPID_I() );
	 break;

	case GET_PID_D_CMD:			// <63>
//		ReplyInt( GetPID_D() );
	 break;

	case GET_PID_SCALE_CMD:		// <64>
//		ReplyInt( GetPID_Scale() );
	 break;

	case GET_PID_ERROR_CMD:		// <65>
//		ReplyInt( GetPID_Error() );
	 break;
	}
}

void servoInit( void )
{
	bDoPID = 0;
	nEncoderPosition = 0;

	InitEncoder();
	InitMotion();
}
