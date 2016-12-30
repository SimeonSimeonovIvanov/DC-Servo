
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "clock.h"
#include "timer.h"

#include <stdbool.h>

/**
 * \addtogroup sys
 *
 * @{
 */
/**
 * \addtogroup sys_clock System clock
 *
 * This module provides an internal clock to other modules. Time is internally
 * represented as the number of seconds since January 1st, 1980 00:00:00 GMT.
 *
 * \note The system clock uses GMT as its base. For display to the user,
 *       some local timezone information might be needed.
 *
 * @{
 */
/**
 * \file
 * System clock implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

static uint32_t clock_current;

static void clock_interval(int timer);

/**
 * Initializes the internal clock system.
 */
void clock_init()
{
    int timer = timer_alloc(clock_interval, 0);
    timer_set(timer, 1000);
}

/**
 * Retrieves current system time.
 *
 * \returns The current system date and time in seconds.
 */
uint32_t clock_get()
{
    return clock_current;
}

/**
 * Adjusts current system time.
 *
 * \param[in] timeval New date and time in seconds.
 */
void clock_set(uint32_t timeval)
{
    clock_current = timeval;
}

/**
 * Calculates a human-readable date and time representation.
 *
 * \param[in] timeval The timestamp in seconds to represent.
 * \param[out] date_time A pointer to the structure which receives the human-readable representation.
 */
void clock_interpret(uint32_t timeval, struct clock_date_time* date_time)
{
    uint16_t days = timeval / (24 * 3600UL);
    timeval -= days * (24 * 3600UL);

    uint16_t years_leap = days / (366 + 3 * 365);
    days -= years_leap * (366 + 3 * 365);
    date_time->year = 1980 + years_leap * 4;
    bool leap = false;
    if(days >= 366)
    {
        days -= 366;
        ++date_time->year;

        if(days >= 365)
        {
            days -= 365;
            ++date_time->year;
        }
        if(days >= 365)
        {
            days -= 365;
            ++date_time->year;
        }
    }
    else
    {
        leap = true;
    }

    date_time->month = 1;
    while(1)
    {
        uint8_t month_days;
        if(date_time->month == 2)
        {
            month_days = leap ? 29 : 28;
        }
        else if(date_time->month <= 7)
        {
            if((date_time->month & 1) == 1)
                month_days = 31;
            else
                month_days = 30;
        }
        else
        {
            if((date_time->month & 1) == 1)
                month_days = 30;
            else
                month_days = 31;
        }

        if(days >= month_days)
        {
            ++date_time->month;
            days -= month_days;
        }
        else
        {
            break;
        }
    }

    date_time->day = days + 1;

    date_time->hour = timeval / 3600;
    timeval -= date_time->hour * 3600UL;

    date_time->minute = timeval / 60;
    timeval -= date_time->minute * 60;

    date_time->second = timeval;
}

/**
 * Timeout callback function for incrementing current system time.
 */
void clock_interval(int timer)
{
    ++clock_current;
    timer_set(timer, 1000);
}

/**
 * @}
 * @}
 */

