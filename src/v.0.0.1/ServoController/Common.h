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

#ifndef __AVR_COMMON_H__
#define __AVR_COMMON_H__

#include <avr/interrupt.h>

#define MotorFault				0x01
#define PositionError			0x02
#define MotorEnabled			0x04

#define NUMBER_SCALE			( 1l<<8 )

typedef uint8_t byte;

#endif

