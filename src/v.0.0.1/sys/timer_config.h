
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TIMER_CONFIG_H
#define TIMER_CONFIG_H

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
 * \addtogroup sys_timer_config Timer service configuration
 *
 * @{
 */
/**
 * \file
 * Timer configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * The maximum number of timers allocated in parallel.
 */
#define TIMER_MAX_COUNT 5

/**
 * The length of the time slices in milliseconds which should be assumed
 * between calls to timer_interval().
 */
#define TIMER_MS_PER_TICK 10

/**
 * @}
 * @}
 * @}
 */

#endif
