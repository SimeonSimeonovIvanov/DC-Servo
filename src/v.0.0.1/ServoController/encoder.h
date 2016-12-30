#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <string.h>
#include <avr/interrupt.h>

#include "Common.h"
#include "../main.h"

//#define __ENCODER_TYPE_3__

#define AB( a, b )		( ( (b)<<1 ) | (a) )
#define ENCODER_A_bm	ID8_INT5_bm
#define ENCODER_B_bm	ID9_INT4_bm

typedef struct {
	int32_t count;
	uint8_t state;
} OPTICAL_ENCODER, *LP_OPTICAL_ENCODER;

void InitEncoder(void);

#endif
