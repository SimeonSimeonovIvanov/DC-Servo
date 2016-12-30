#include "current_loop.h"

void current_loop_update( int16_t pwm, uint8_t direction )
{
	uint8_t pwm_width = (uint8_t)pwm;

	if( !direction ) {
#if !SWAP_PWM_DIRECTION_ENABLED
		pwm_dir_a( pwm_width );
#else
		pwm_dir_b( pwm_width );
#endif
	} else {
#if !SWAP_PWM_DIRECTION_ENABLED
		pwm_dir_b( pwm_width );
#else
		pwm_dir_a( pwm_width );
#endif
	}
}
