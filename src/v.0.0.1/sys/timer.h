
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "timer_config.h"

/**
 * \addtogroup sys
 *
 * @{
 */
/**
 * \addtogroup sys_timer
 *
 * @{
 */
/**
 * \file
 * Timer header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * The type of callback function executed when a timer expires.
 */
typedef void (*timer_callback)(int timer);

void timer_interval();

int timer_alloc(timer_callback callback, uintptr_t user);
void timer_free(int timer);

bool timer_set(int timer, uint32_t millis);
bool timer_stop(int timer);
bool timer_expired(int timer);

uintptr_t timer_get_user(int timer);
bool timer_set_user(int timer, uintptr_t user);

#define timer_valid(timer) (((unsigned int) (timer)) < TIMER_MAX_COUNT)

/**
 * @}
 * @}
 */

#endif

