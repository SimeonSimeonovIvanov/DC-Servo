#include "encoder.h"

#define __ENCODER_TYPE_UP_DOWN_COUNTER__X1__

volatile int32_t nEncoderPosition;

static int8_t Map[4][4];
static volatile int8_t nEncoderOld;
static volatile OPTICAL_ENCODER opticalEncoder;

void InitEncoder(void)
{
	DDRE &= ~( ENCODER_A_bm | ENCODER_B_bm );

	memset( &Map, 0, sizeof(Map) );
	memset( (void*)&opticalEncoder, 0, sizeof(opticalEncoder) );

	Map[ AB(1,0) ][ AB(1,1) ] = -1;
	Map[ AB(1,0) ][ AB(0,0) ] = +1;
	Map[ AB(1,1) ][ AB(0,1) ] = -1;
	Map[ AB(1,1) ][ AB(1,0) ] = +1;
	Map[ AB(0,1) ][ AB(0,0) ] = -1;
	Map[ AB(0,1) ][ AB(1,1) ] = +1;
	Map[ AB(0,0) ][ AB(1,0) ] = -1;
	Map[ AB(0,0) ][ AB(0,1) ] = +1;

	nEncoderPosition = 0;
	nEncoderOld = PINE & ( ENCODER_A_bm | ENCODER_B_bm );

#ifdef __ENCODER_TYPE_X4__ // x4 +++

	// Any logical change on INTn generates an interrupt request:
	EICRB = 1<<ISC50 | 1<<ISC40;
	EIMSK = 1<<INT5  | 1<<INT4;

#elif defined __ENCODER_TYPE_1__ // ???

	// The rising edge between two samples of INTn generates an interrupt request:
	EICRB = 1<<ISC51 | 1<<ISC50 | 1<<ISC41 | 1<<ISC40;
	EIMSK = 1<<INT5 | 1<<INT4;

#elif defined __ENCODER_TYPE_2__ // ???

	// The rising edge between two samples of INTn generates an interrupt request:
	EICRB = 1<<ISC51 | 1<<ISC50 | 0<<ISC41 | 0<<ISC40;
	EIMSK = 1<<INT5 | 0<<INT4;

#elif defined __ENCODER_TYPE_3__ // ???

	// The rising edge between two samples of INTn generates an interrupt request:
	EICRB = 1<<ISC51 | 1<<ISC50 | 1<<ISC41 | 1<<ISC40;
	EIMSK = 1<<INT5 | 1<<INT4;

#elif defined __ENCODER_TYPE_UP_DOWN_COUNTER__X1__

	// The rising edge between two samples of INTn generates an interrupt request:
	EICRB = 1<<ISC51 | 1<<ISC50 | 1<<ISC41 | 1<<ISC40;
	EIMSK = 1<<INT5 | 1<<INT4;

#elif defined __ENCODER_TYPE_UP_DOWN_COUNTER__X2__ // ???

		// Any logical change on INTn generates an interrupt request:
	EICRB = 1<<ISC50 | 1<<ISC40;
	EIMSK = 1<<INT5  | 1<<INT4;

#endif
}

#ifdef __ENCODER_TYPE_X4__

ISR( INT4_vect )
{
	volatile uint8_t nEncoderNew = ( PINE & ( ENCODER_A_bm | ENCODER_B_bm ) )>>4;
	volatile int8_t n = Map[ nEncoderNew ][ nEncoderOld ];

	nEncoderPosition -= n;
	nEncoderOld = nEncoderNew;
}

ISR( INT5_vect )
{
	volatile uint8_t nEncoderNew = ( PINE & ( ENCODER_A_bm | ENCODER_B_bm ) )>>4;
	volatile int8_t n = Map[ nEncoderNew ][ nEncoderOld ];

	nEncoderPosition -= n;
	nEncoderOld = nEncoderNew;
}

#elif defined __ENCODER_TYPE_1__

ISR( INT4_vect )
{
	switch( opticalEncoder.state ) {
	case 0:
		if( PINE & ENCODER_A_bm ) {
			opticalEncoder.state = 21;
		}
	 break;

	case 11:
		--nEncoderPosition;
		//opticalEncoder.count -= 1;
		opticalEncoder.state = 0;
	 break;

	case 21:
		++nEncoderPosition;
		opticalEncoder.state = 0;
	 break;
	
	default: {
		opticalEncoder.state = 0;
	 }
	}
}

ISR( INT5_vect )
{
	switch( opticalEncoder.state ) {
	case 0:
		if( PINE & ENCODER_B_bm ) {
			opticalEncoder.state = 11;
		}
	 break;

	case 21:
		++nEncoderPosition;
		//opticalEncoder.count += 1;
		opticalEncoder.state = 0;
	 break;

	case 11:
		--nEncoderPosition;
		opticalEncoder.state = 0;
	 break;

	default: {
		opticalEncoder.state = 0;
	 }
	}
}

#elif defined __ENCODER_TYPE_2__

ISR( INT4_vect )
{
	if( PINE & ENCODER_A_bm ) {
		++nEncoderPosition;
	} else {
		--nEncoderPosition;
	}
}

ISR( INT5_vect )
{
	if( PINE & ENCODER_B_bm ) {
		--nEncoderPosition;
	} else {
		++nEncoderPosition;
	}
}

#elif defined __ENCODER_TYPE_3__

volatile uint8_t TR_A = 0, TR_B = 0;
volatile uint8_t TR_A_OLD = 0, TR_B_OLD = 0;

// B
ISR( INT4_vect )
{
	if( !( PINE & ENCODER_A_bm ) ) {
		TR_A = 0;
	}

	if( !( PINE & ENCODER_B_bm ) ) {
		TR_B = 0;
	}
	////////////////////////////////

	if( TR_A != TR_A_OLD ) {
		++nEncoderPosition;
		TR_A = TR_A_OLD;
	}

	if( ( PINE & ENCODER_A_bm ) ) {
		TR_A = 1;
	}
}

// A
ISR( INT5_vect )
{
	if( !( PINE & ENCODER_A_bm ) ) {
		TR_A = 0;
	}

	if( !( PINE & ENCODER_B_bm ) ) {
		TR_B = 0;
	}
	////////////////////////////////

	if( TR_B != TR_B_OLD ) {
		--nEncoderPosition;
		TR_B = TR_B_OLD;
	}

	if( ( PINE & ENCODER_B_bm ) ) {
		TR_B = 1;
	}
}

#elif defined __ENCODER_TYPE_UP_DOWN_COUNTER__X1__ || defined __ENCODER_TYPE_UP_DOWN_COUNTER__X2__

ISR( INT4_vect )
{
	++nEncoderPosition;
}

ISR( INT5_vect )
{
	--nEncoderPosition;
}

#endif
