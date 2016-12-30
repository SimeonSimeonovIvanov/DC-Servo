#ifndef __MOTION_H__
#define __MOTION_H__

#include <stdlib.h>
#include <inttypes.h>
#include <avr/eeprom.h>

#include "Common.h"

#define TIME_PERIOD		1024l		// Motion time uints are in 1/1024 of a second

typedef int32_t motion_t;
typedef uint32_t umotion_t;

enum EState
{
	eStopped,
	eAccel,
	eRun,
	eDecel,
	eDecel2
};

#define sign(x)		( (x) < 0 ? -1 : (x) == 0 ? 0 : 1)


uint8_t Moving(void);
void InitMotion(void);
void InitMotinForFirstRun( void );
void SetAccAndMaxVelocity( motion_t nAcc, motion_t nMaxVel );

void MotionUpdate(void);
void Move(motion_t nMovement);
void MoveTo(motion_t nPosition);
void motionSetRunState(enum EState newRunState);

void SetSpeedPeriod( int32_t ms );
void SetMaxSpeed( int32_t nSpeed );
void SetAcceleration( int32_t nAcc );

void SetVelocity( motion_t nVelocity );

void MotionReadEeprom(void);
void MotionSaveEeprom(void);

unsigned long isqrt(umotion_t number);

motion_t motionGetCurrentVelocity( void );
void motionSetCurrentVelocity( motion_t nNewVelocity );

motion_t motionGetTargetPosition( void );
motion_t motionGetCurrentPosition( void );
void motionSetTargetPosition( motion_t nNewPosition );
void motionSetCurrentPosition( motion_t nNewPosition );

#endif
