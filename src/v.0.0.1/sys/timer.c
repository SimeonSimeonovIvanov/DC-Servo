
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "timer.h"
#include "timer_config.h"

#include <stdint.h>

/**
 * \addtogroup sys Generic system services
 *
 * These modules provide generic services used in the whole system.
 *
 * @{
 */
/**
 * \addtogroup sys_timer Timer service
 *
 * This module provides timeout callbacks.
 *
 * @{
 */
/**
 * \file
 * Timer implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

#define TIMER_FLAG_STOPPED 1

struct timer_descriptor
{
	uint8_t flags;
	uint32_t next;
	uintptr_t user;
	timer_callback callback;	
};

static struct timer_descriptor timer_descriptors[TIMER_MAX_COUNT];
static uint32_t timer_clock;

#define FOREACH_TIMER(timer)								\
	for( (timer) = &timer_descriptors[0];					\
		 (timer) < &timer_descriptors[TIMER_MAX_COUNT];		\
		 ++(timer)											\
	)

static int timer_number(const struct timer_descriptor* timer);

/**
 * Global timeout handling function.
 *
 * This function must be continuously called once for every time slice. This is usually
 * done from the application main loop. However, it should not be done from an interrupt
 * service routine to avoid race conditions.
 *
 * The accuracy of the timer service is principally dependent on the accuracy of the
 * timer interval between calls of this function. So do your best to guarantee constant
 * intervals.
 *
 * The length of the time slice is configured with #TIMER_MS_PER_TICK.
 */
void timer_interval()
{
	struct timer_descriptor* timer;

	++timer_clock;

	FOREACH_TIMER(timer) {
		if( !timer->callback || (TIMER_FLAG_STOPPED & timer->flags) ) {
			continue;
		}

		if( timer->next == timer_clock ) {
			timer->flags |= TIMER_FLAG_STOPPED;
			timer->callback(timer_number(timer));
		}
	}
}

/**
 * Allocates a timer.
 *
 * The given callback function is executed whenever the timer expires. The \a user
 * value is attached to the timer and given as an extra argument to the callback
 * function. It is not used by the timer module in any way.
 *
 * \param[in] callback The function to execute whenever the timer expires.
 * \param[in] user The user-defined value.
 * \returns A non-negative timer identifier on success, \c -1 on failure.
 */
int timer_alloc(timer_callback callback, uintptr_t user)
{
	struct timer_descriptor* timer;

	if(!callback) {
		return -1;
	}

	FOREACH_TIMER(timer) {
		if(timer->callback) {
			continue;
		}

		timer->callback = callback;
		timer->user = user;
		timer->flags = TIMER_FLAG_STOPPED;

		return timer_number(timer);
	}

	return -1;
}

/**
 * Deallocates a timer.
 *
 * The timer being deallocated is stopped and marked as free.
 *
 * \param[in] timer The identifier of the timer which to deallocate.
 */
void timer_free(int timer)
{
	if(!timer_valid(timer)) {
		return;
	}

	timer_descriptors[timer].callback = 0;
}

/**
 * Starts or restarts a timer.
 *
 * The timer expires after \a millis milliseconds.
 *
 * \note Timers do not automatically restart. Call this function from the
 *	   timeout callback function to restart the timer.
 *
 * \param[in] timer The identifier of the timer which to start.
 * \param[in] millis The number of milliseconds after which the timer will expire.
 * \returns \c true if the timer has successfully been started, \c false on failure.
 */
bool timer_set(int timer, uint32_t millis)
{
	struct timer_descriptor* t;

	if(!timer_valid(timer))
		return false;

	t = &timer_descriptors[timer];
	if(!t->callback) {
		return false;
	}

	t->next = timer_clock + (millis + TIMER_MS_PER_TICK - 1) / TIMER_MS_PER_TICK;
	t->flags &= ~TIMER_FLAG_STOPPED;

	return true;
}

/**
 * Stops the timer.
 *
 * \param[in] timer The identifier of the timer which to stop.
 * \returns \c true if the timer has been stopped, \c false on failure.
 */
bool timer_stop(int timer)
{
	if(!timer_valid(timer)) {
		return false;
	}

	timer_descriptors[timer].flags |= TIMER_FLAG_STOPPED;
	return true;
}

/**
 * Checks wether a timer has expired or was stopped manually.
 *
 * \param[in] timer The identifier of the timer which to check.
 * \returns \c true if the timer is not running, \c false otherwise.
 */
bool timer_expired(int timer)
{
	struct timer_descriptor* t;

	if(!timer_valid(timer))
		return false;

	t = &timer_descriptors[timer];
	if(!t->callback) {
		return false;
	}

	return (t->flags & TIMER_FLAG_STOPPED) != 0;
}

/**
 * Retrieves the user-defined data attached to a timer.
 *
 * \param[in] timer The identifier of the timer of which to retrieve the data.
 * \returns The user-defined data of the timer on success, \c 0 on failure.
 */
uintptr_t timer_get_user(int timer)
{
	struct timer_descriptor* t;

	if(!timer_valid(timer)) {
		return 0;
	}

	t = &timer_descriptors[timer];
	if(!t->callback) {
		return 0;
	}

	return t->user;
}

/**
 * Assigns user-defined data to a timer.
 *
 * \param[in] timer The identifier of the timer which to assign the data to.
 * \param[in] user The data which to assign to the timer.
 * \returns \c true on success, \c false on failure.
 */
bool timer_set_user(int timer, uintptr_t user)
{
	struct timer_descriptor* t;
	
	if(!timer_valid(timer)) {
		return false;
	}

	t = &timer_descriptors[timer];
	if(!t->callback) {
		return false;
	}

	t->user = user;
	return true;
}

/**
 * Retrieves the timer identifier of an internal timer structure.
 *
 * \param[in] timer The internal timer structure for which to retrieve the identifier.
 * \returns The non-negative timer identifier on success, \c -1 on failure.
 */
int timer_number(const struct timer_descriptor* timer)
{
	int t;

	if(!timer) {
		return -1;
	}

	t  = ((int) timer - (int) &timer_descriptors[0]) / sizeof(*timer);

	if(!timer_valid(t)) {
		return -1;
	}

	return t;
}

/**
 * @}
 * @}
 */
