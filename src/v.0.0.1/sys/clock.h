
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

/**
 * \addtogroup sys
 *
 * @{
 */
/**
 * \addtogroup sys_clock
 *
 * @{
 */
/**
 * \file
 * System clock header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * Representation of human-readable date and time.
 */
struct clock_date_time
{
    /** The four-digit year. */
    uint16_t year;
    /** The month (1-12). */
    uint8_t month;
    /** The day (1-31). */
    uint8_t day;
    /** The hour (0-23). */
    uint8_t hour;
    /** The minute (0-59). */
    uint8_t minute;
    /** The second (0-59). */
    uint8_t second;
};

void clock_init();
uint32_t clock_get();
void clock_set(uint32_t timeval);

void clock_interpret(uint32_t timeval, struct clock_date_time* date_time);

/**
 * @}
 * @}
 */

#endif

