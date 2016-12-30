#include "motion.h"

static enum EState nRunState;

static EEMEM uint8_t firstRunMotinEEPROM;
static EEMEM motion_t eeVelocityMax;
static EEMEM motion_t eeAcceleration;
static EEMEM motion_t eeRunTimePeriod;

// Configuration constants
static volatile motion_t nVelocityMax;               // Max velocity in steps/(1/1024s)    - steps/s^2
static volatile motion_t nAcceleration;              // Acceleration in steps/(1/1024s)^2  - steps/s
static volatile motion_t nVelocityPeriod;            // Time a velocity move will keep going for.
static volatile motion_t nMaxAccelerationTime;       // time to accelerate from 0 to max velocity
static volatile motion_t nMaxAccelerationDistance;   // distance travelled accelerating from 0 to Vmax

// Run time variables
static volatile motion_t nTargetPosition;            // final target position of current active motion path
static volatile motion_t nCurrentPosition;           // actual commanded position
static volatile motion_t nCurrentVelocity;           // current velocity
static volatile motion_t nCurrentAcceleration;       // current velocity

static volatile umotion_t nAccTime;
static volatile umotion_t nRunTime;
static volatile umotion_t nDecTime;
static volatile umotion_t nDecTime2;
static volatile umotion_t nRunTimeFraction;

static void DoMove(motion_t nMovement);

void Move(motion_t nMovement)
{
	DoMove( nMovement * NUMBER_SCALE );
}

void MoveTo(motion_t nPosition)
{
	DoMove( (nPosition * NUMBER_SCALE) - nTargetPosition );
}

static void DoMove(motion_t nMovement)      // Do the path planning
{
	uint8_t bReverse = 0;

	// Math is based on basic motion equations...
	// v = v0 + at
	// x = x0 + v0t + 1/2 a t^2
	//
	// t = time
	// a = acceleration, which is constant for all our motion
	// v = velocity
	// v0 = initial velocity
	// x = distance
	// x0 = initial distance
	//
	// But, because we are using integer math, we need to make some adjustments.
	// Normally, v vs t is a straight line, but because we use integer math, the result is a staircase...
	//
	// * x0 = 0 and v0 = 0
	//
	//         ^                           ^                        ^
	//       v |       /                 v |       /|             v ||\                            .
	//         |      /                    |      /_|               ||_\                           .
	//         |     /                     |     /|                 |  |\                          .
	//         |    /                      |    /_|                 |  |_\                         .
	//         |   /                       |   /|                   |    |\                        .
	//         |  /                        |  /_|                   |    |_\                       .
	//         | /                         | /|                     |      |\                      .
	//         |/                          |/ |                     |      |_\                     .
	//         +---------->                +---------->             +---------->
	//                   t                           t                        t
	//
	// this doesnt effect, v = at, but x = 1/2at^2 now becomes x = 1/2at^2 - 1/2at

	if( !nMovement ) {
		return;
	}

	if( nCurrentVelocity < 0 ) {
		bReverse = 1;
	}

	if( nRunState != eStopped ) {	
		uint8_t bTurnAround = 0;
		motion_t nDecelFromCurVelTime;
		motion_t nDecelFromCurVelDistance;

		// Special conditions - handle already moving separately.  Maths is a bit more complicated, with an initial velocity, so don't
		// do it and bog down the microcontroller if we don't have to.

		// moving.  Need to combine the current target with this move
		nMovement += nTargetPosition - nCurrentPosition;

		// Determine the distance we will travel if we come to a stop now.  This will be handled in the acceleration phase, but with a 
		// negative acceleration value, so we use x = 1/2at^2 + 1/2at
		//
		//         ^                        ^                                                      .
		//       v |       /|             v |\ |                                                   .
		//         |      /_|               | \|_                                                  .
		//         |     /|                 |  \ |                                                 .
		//         |    /_|                 |   \|_                                                .
		//         |   /|                   |    \ |                                               .
		//         |  /_|                   |     \|_                                              .
		//         | /|                     |      \ |                                             .
		//         |/ |                     |       \|_                                            .
		//         +---------->             +---------->                                           .
		//                   t                        t                                            .
		//

		nDecelFromCurVelTime = labs(nCurrentVelocity)/nAcceleration;
		nDecelFromCurVelDistance = sign(nCurrentVelocity) * nAcceleration * (nDecelFromCurVelTime+1l) * nDecelFromCurVelTime / 2l;

		if( nCurrentVelocity > 0 ) {
			if( nDecelFromCurVelDistance  > nMovement ) {
				bTurnAround = 1;
			}
		} else {
			if( nDecelFromCurVelDistance < nMovement ) {
				bTurnAround = 1;
			}
		}

		// we are running too fast and cant stop using a regular trap/tri profile
		if( bTurnAround ) {
			// There are 2 parts to this.  1) Decelerate to V=0, 2) Either Triangle or Trapezoid to final spot.
			// We can combine 1) decelerate and 2) accelerate as they are accelerating at -nAcceleration
			// Profiles are easy because after we decelerate, Vi=0

			motion_t nLength = labs(nMovement - nDecelFromCurVelDistance);

			// now determine the tri/trap
			if (nLength > 2 * nMaxAccelerationDistance )	// we fit the trapezoid profile - Accelerate to max, run, then decelerate.
			{
				nAccTime = nMaxAccelerationTime;
				nDecTime = nMaxAccelerationTime;
				nRunTime = (nLength - 2 * nMaxAccelerationDistance) / nVelocityMax;

				// Calculate the run time fraction. This is the left over fractions to stop exactly.
				nRunTimeFraction = (nLength - 2 * nMaxAccelerationDistance) % nVelocityMax;
			} else {
				motion_t nPeakSpeed, nPeakVelocity, nAccelerationDistance;

				// Need to solve motion equations to find time to accelerate, then decelerate to cover the distance.
				// Since it is a symetrical path, find x/2.
				// We know
				//       x               1    2   1
				//       - = x0 + V0 t + - a t  - - a t
				//       2               2        2
				//
				// with x0 = 0, V0 - 0,
				//
				// leaves
				//       x   1    2   1
				//       - = - a t  - - a t
				//       2   2        2
				//
				//           V
				// and   t = -
				//           a
				//
				//
				// Solve for V...
				//              +---------+
				//          1   |        2    1
				//       V= -   |4a x + a   + - a
				//          2  \|             2

				nPeakSpeed = (isqrt( 4 * nAcceleration * nLength + nAcceleration * nAcceleration) + nAcceleration/2)/2;
				
				// make sure peak velocity is a multiple of the acceleration.
				nPeakVelocity = (nPeakSpeed / nAcceleration) * nAcceleration;
				
				// catch really small velocities (small move).
				if( !nPeakVelocity ) {
					nPeakVelocity = nAcceleration;
				}

				nAccTime = nPeakVelocity / nAcceleration;
				nDecTime = nAccTime;
				
				nAccelerationDistance = nAcceleration * nAccTime * ( nAccTime - 1l ) / 2l;
				nRunTime = ( nLength - 2 * nAccelerationDistance ) / nPeakVelocity;		

				// Calculate the run time fraction.
				nRunTimeFraction = (nLength - 2*nAccelerationDistance) % nPeakVelocity;
			}

			nAccTime += nDecelFromCurVelTime;
			bReverse = !bReverse;

			/*
			if ( nMovement < 0 ) {
				bReverse = 1;
			} else {
				bReverse = 0;
			}
			*/
		} else {
			// standard motion, except V0 != 0

			// Calculate time and distance to accelerate from V0 to Vmax
			//
			//
			//     Vmax - V0
			// t = ---------
			//         a
			//
			// and
			//
			//     1    2   1
			// x = - a t  - - a t + V0 t
			//     2        2

			motion_t nLength;
			motion_t nAccelToMaxVelTime;
			motion_t nAccelToMaxDistance;

			nLength = labs( nMovement );
			nAccelToMaxVelTime = ( nVelocityMax - labs( nCurrentVelocity ) ) / nAcceleration;
			
			nAccelToMaxDistance = (
				nAcceleration * ( nAccelToMaxVelTime - 1l ) *
				nAccelToMaxVelTime / 2l +
				labs(nCurrentVelocity) * nAccelToMaxVelTime
			);

			// if the accel distance (to Vmax) + decel distance is less than the total distance, we can accelerate to max speed, coast, then decelerate.
			if( nLength > nAccelToMaxDistance + nMaxAccelerationDistance )	// trapezoid profile - Accelerate to max, run, then decelerate.
			{
				// We fiddle with the run time to get the correct distance.
				nAccTime = nAccelToMaxVelTime;
				nDecTime = nMaxAccelerationTime;
				nRunTime = (nLength - nAccelToMaxDistance - nMaxAccelerationDistance) / nVelocityMax;

				// Calculate the run time fraction.
				nRunTimeFraction = (nLength - nAccelToMaxDistance - nMaxAccelerationDistance) % nVelocityMax;
			} else { // triangle profile

				// Now it gets a bit nasty.  Above when we solved for V, with V0 = 0, it was pretty simple.  Now we have V0, so we need to find Vpeak below.
				//
				//         ^                                                                       .
				//   Vpeak |---^                                                                   .
				//         |  /|\                                                                  .
				//         | / | \                                                                 .
				//       V0|/  |  \                                                                .
				//         |   |   \                                                               .
				//         | x1| x2 \                                                              .
				//         |   |     \                                                             .
				//         |   |      \                                                            .
				//         +---------------->                                                      .
				//         <t1>|<- t2->t                                                           .
				//
				// We know...
				// t1 = (V-V0)/a
				// t1 = V/a
				// x1 = V0 t1 + 1/2 a t1^2 - 1/2*a*t1
				// x2 = 1/2 a t2^2 - 1/2*a*t2
				// x  = x1 + x2
				//
				// Solving with for v gives...
				//
				//         +------------------------+
				//    + 1  |          2            2  1
				// v= - - \|4a x + 2v0  - 2a v0 + a + - a
				//      2							  2

				motion_t nPeakSpeed = isqrt( 4 * nAcceleration * nLength + 2 * nCurrentVelocity * nCurrentVelocity - 2 * nAcceleration * labs(nCurrentVelocity)  + nAcceleration * nAcceleration) / 2 + nAcceleration/2;
				//motion_t nPeakSpeed = isqrt( 4 * nAcceleration * nLength + 4 * nCurrentVelocity * nCurrentVelocity - 4 * nAcceleration * labs(nCurrentVelocity)  + nAcceleration * nAcceleration) / 2 - labs(nCurrentVelocity) + nAcceleration/2;

				// make sure peak velocity is a multiple of the acceleration.
				motion_t nPeakVelocity = (nPeakSpeed / nAcceleration) * nAcceleration;
				
				// Why?
				if( nPeakVelocity < labs(nCurrentVelocity) ) {
					nPeakVelocity = labs(nCurrentVelocity);
				}
				
				// catch really small velocities (small move)
				if( !nPeakVelocity ) {
					nPeakVelocity = nAcceleration;
				}
				
				nAccTime = (nPeakVelocity - labs(nCurrentVelocity)) / nAcceleration;
				nDecTime = nPeakVelocity / nAcceleration;
				motion_t nAccelerationDistance = nAccTime * labs(nCurrentVelocity) + nAcceleration * nAccTime * (nAccTime - 1l)/2l;
				motion_t nDecelDistance = nAcceleration * nDecTime * (nDecTime - 1l )/2l;
				nRunTime = (nLength - nAccelerationDistance - nDecelDistance) / nPeakVelocity;

				// Calculate the run time fraction.
				nRunTimeFraction = labs((nLength - nAccelerationDistance - nDecelDistance) % nPeakVelocity);
			}
		}
	} else {
		if( nMovement < 0 ) {
			bReverse = 1;
		}

		// Simple V0 = 0
		motion_t nLength = labs(nMovement);

		// we fit the trapezoid profile - Accelerate to max, run, then decelerate.
		if( nLength > 2 * nMaxAccelerationDistance ) {
			// We fiddle with the run time to get the correct distance.
			nAccTime = nMaxAccelerationTime;
			nDecTime = nAccTime;
			nRunTime = ( nLength - 2 * nMaxAccelerationDistance ) / nVelocityMax;

			// Calculate the run time fraction.
			nRunTimeFraction = ( nLength - 2 * nMaxAccelerationDistance ) % nVelocityMax;
		} else { // triangle profile
			// We won't get to max without overruning the end.
			motion_t nPeakSpeed = isqrt( 4 * nAcceleration * nLength + nAcceleration * nAcceleration - nAcceleration/2)/2;

			motion_t nPeakVelocity = (nPeakSpeed / nAcceleration) * nAcceleration;	// make sure peak velocity is a multiple of the acceleration.

			// catch really small velocities (small move).
			if( !nPeakVelocity ) {
				nPeakVelocity = nAcceleration;
			}

			nAccTime = nPeakVelocity / nAcceleration;
			nDecTime = nAccTime;
			motion_t nAccelerationDistance = nAcceleration * nAccTime * (nAccTime - 1l)/2l;
			nRunTime = (nLength - 2 * nAccelerationDistance ) / nPeakVelocity;

			// Calculate the run time fraction.
			nRunTimeFraction = (nLength - 2 * nAccelerationDistance )  % nPeakVelocity;
		}
	}

	if( nAccTime > 0 ) {
		nRunState = eAccel;
	} else {
		nRunState = eRun;
	}
	
	nDecTime2 = 0;

	if( bReverse ) {
		nCurrentAcceleration = -1 * nAcceleration;
		nRunTimeFraction = -1 * nRunTimeFraction;
	} else {
		nCurrentAcceleration = nAcceleration;
	}
	
	nTargetPosition = nCurrentPosition + nMovement;

#ifdef TEST
	{
		motion_t x = 0;

		x += (motion_t)nAccTime * ((motion_t)nAccTime-1) * nCurrentAcceleration / 2 + nCurrentVelocity * (motion_t)nAccTime;
		motion_t nPeakVelocity = nCurrentVelocity + nCurrentAcceleration * (motion_t)nAccTime;
		x += (motion_t)nRunTime * nPeakVelocity;
		x += (motion_t)nDecTime * ((motion_t)nDecTime-1) * nCurrentAcceleration / 2;
		x += nRunTimeFraction;

		assert( x == nMovement );
	}
#endif
}

void SetVelocity( motion_t nVelocity )
{
	nVelocity *= NUMBER_SCALE;

	// Velocity must be a multiple of acceleration.
	nVelocity = (nVelocity / nAcceleration) * nAcceleration;

	if( ( nVelocity > 0 && nCurrentVelocity > 0 && nVelocity < nCurrentVelocity ) ||
		( nVelocity < 0 && nCurrentVelocity < 0 && nVelocity > nCurrentVelocity )
	) {
		// this is a slow down, then stop.
		// Need to handle it specially because there are 2 decelerations - dec, run, dec
		// We handle this as 2 negative accelerations.

		nAccTime = labs(nVelocity - nCurrentVelocity) / nAcceleration;
		nRunTime = nVelocityPeriod;
		nRunTimeFraction = 0;
		nDecTime = 0;
		nDecTime2 = labs(nVelocity) / nAcceleration;

		if( nVelocity > 0 ) {
			nCurrentAcceleration = -nAcceleration;
		} else {
			nCurrentAcceleration = nAcceleration;
		}

		if( nAccTime > 0 ) {
			nRunState = eAccel;
		} else {
			nRunState = eRun;
		}
	} else {
		nAccTime = labs(nVelocity - nCurrentVelocity) / nAcceleration;
		nRunTime = nVelocityPeriod;
		nRunTimeFraction = 0;
		nDecTime = labs(nVelocity)/nAcceleration;
		nDecTime2 = 0;

		if( nVelocity < 0 ) {
			nCurrentAcceleration = -nAcceleration;
		} else {
			nCurrentAcceleration = nAcceleration;
		}

		if( nAccTime > 0 ) {
			nRunState = eAccel;
		} else {
			nRunState = eRun;
		}
	}
}

void MotionUpdate(void)
{
	switch(nRunState) {
	case eAccel:
		nAccTime--;
		nCurrentPosition += nCurrentVelocity;
		nCurrentVelocity += nCurrentAcceleration;

		if( !nAccTime ) {
			if( !nRunTime ) {
				nRunState = eDecel;
			} else {
				nRunState = eRun;
			}
		}
	 break;

	case eRun:
		nRunTime--;
		nCurrentPosition += nCurrentVelocity;

		if( !nRunTime ) {
			if( nDecTime2 ) {
				nRunState = eDecel2;
			} else {
				nRunState = eDecel;
			}
		}
	 break;

	case eDecel:
		// We stuff the fractional left over bit into the deceleration.  We wait until the velocity is about the same then squeeze it in.
		if( nRunTimeFraction && labs(nRunTimeFraction) > labs(nCurrentVelocity) ) {
			nCurrentPosition += nRunTimeFraction;
			nRunTimeFraction = 0;
		} else {
			if( nDecTime ) {
				nDecTime--;
			}
	
			if( !nDecTime && nRunTimeFraction ) {
				nCurrentPosition += nRunTimeFraction;
				nRunTimeFraction = 0;
			} else {
				nCurrentVelocity -= nCurrentAcceleration;
				nCurrentPosition += nCurrentVelocity;

				if( !nDecTime ) {
					nRunState = eStopped;
				}
			}
		}
	 break;

	case eDecel2:	
		// Special for 2 step deceleration for velocity motion.
		nDecTime2--;

		nCurrentVelocity += nCurrentAcceleration;	// we're adding a negative acceleration here.
		nCurrentPosition += nCurrentVelocity;

		if( !nDecTime2 ) {
			nRunState = eStopped;
		}
	 break;

	case eStopped:
	 break;
	}
}

uint8_t Moving(void)
{
	return nRunState != eStopped;
}

void motionSetRunState(enum EState newRunState)
{
	nRunState = newRunState;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void SetAccAndMaxVelocity( motion_t nAcc, motion_t nMaxVel )
{
	nAcc *= TIME_PERIOD;		// steps/sec/sec
	nMaxVel *= TIME_PERIOD;		// steps/sec

	// scaled steps / scaled_time ^ 2
	nAcceleration = nAcc * NUMBER_SCALE / TIME_PERIOD / TIME_PERIOD;

	// scaled steps / scaled_time
	nVelocityMax = nMaxVel * NUMBER_SCALE / TIME_PERIOD;

	// Round down the max velocity to a multiple of acceleration
	nVelocityMax = ( nVelocityMax / nAcceleration ) * nAcceleration;

	nMaxAccelerationTime = nVelocityMax / nAcceleration;

	// Normally we use x = 1/2 at^2, but because we do a stair case effect, we remove 1/2at
	// x = 1/2 at^2 - 1/2at 
	// x = 1/2 at(t-1)
	nMaxAccelerationDistance = nAcceleration * ( nMaxAccelerationTime - 1l ) * nMaxAccelerationTime / 2l;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void InitMotion(void)
{
	if( 0xAA != eeprom_read_byte((void*)&firstRunMotinEEPROM) ) {
		InitMotinForFirstRun();
		MotionSaveEeprom();
		eeprom_update_byte( (void*)&firstRunMotinEEPROM, 0xAA );
	}

	MotionReadEeprom();

	nCurrentPosition = 0;
	nTargetPosition = 0;

	SetAccAndMaxVelocity( 150, 35);

	nRunState = eStopped;
}

void InitMotinForFirstRun( void )
{
	SetSpeedPeriod( 1000 );
	SetMaxSpeed( 35l* 1024l );
	SetAcceleration( 150l * 1024l );
}

void MotionReadEeprom(void)
{
	eeprom_read_block( (motion_t*)&nVelocityMax, &eeVelocityMax, sizeof(nVelocityMax) );
	eeprom_read_block( (motion_t*)&nAcceleration, &eeAcceleration, sizeof(nAcceleration) );
	eeprom_read_block( (motion_t*)&nVelocityPeriod, &eeRunTimePeriod, sizeof(nVelocityPeriod) );
}

void MotionSaveEeprom(void)
{
	eeprom_write_block( (motion_t*)&nVelocityPeriod, &eeRunTimePeriod, sizeof(eeRunTimePeriod) );
	eeprom_write_block( (motion_t*)&nAcceleration, &eeAcceleration, sizeof(eeAcceleration) );
	eeprom_write_block( (motion_t*)&nVelocityMax, &eeVelocityMax, sizeof(eeVelocityMax) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void SetSpeedPeriod( int32_t ms )
{
	// period is in ms.
	// convert to time_period
	nVelocityPeriod = ms;
	nVelocityPeriod *= TIME_PERIOD;
	nVelocityPeriod /= 1000;
}

void SetMaxSpeed( int32_t nSpeed )
{
	// speed is in steps/s
	// convert to (scaled steps)/time_period
	nVelocityMax = nSpeed;
	nVelocityMax *= NUMBER_SCALE;
	nVelocityMax /= TIME_PERIOD;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void SetAcceleration( int32_t nAcc )		
{
	// acceleration is in steps/s/s.
	// convert to (scaled steps)/time_period/time_period
	nAcceleration = nAcc;
	nAcceleration *= NUMBER_SCALE;
	nAcceleration /= TIME_PERIOD;
	nAcceleration /= TIME_PERIOD;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void motionSetCurrentVelocity( motion_t nNewVelocity )
{
	nCurrentVelocity = nNewVelocity;
}

motion_t motionGetCurrentVelocity( void )
{
	return nCurrentVelocity;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void motionSetCurrentPosition( motion_t nNewPosition )
{
	nCurrentPosition = nNewPosition;
}

motion_t motionGetCurrentPosition( void )
{
	return nCurrentPosition;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void motionSetTargetPosition( motion_t nNewTargetPosition )
{
	nTargetPosition = nNewTargetPosition;
}

motion_t motionGetTargetPosition( void )
{
	return nTargetPosition;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long isqrt(umotion_t number) 
{
	umotion_t G = 0;
	
	umotion_t mask = 0x8000;
	while ( mask != 0 )
	{
		umotion_t tempG = G | mask;

		umotion_t G2 = tempG * tempG;

		if ( number > G2 )
		{
			G = tempG;
		}
		mask >>= 1;
	}
	return G;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
