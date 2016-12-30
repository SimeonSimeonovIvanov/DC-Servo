
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "clock_sync.h"
#include "clock_sync_config.h"

#include "../net/tcp.h"
#include "../sys/clock.h"
#include "../sys/timer.h"

#include <stdbool.h>

/**
 * \addtogroup app
 *
 * @{
 */
/**
 * \addtogroup app_clock_sync Clock synchronization over the Internet
 * A simple TIME protocol client for synchronizing date and time from remote hosts.
 *
 * This client implements the TCP version of the protocol of RFC 868.
 *
 * \note In the official firmware my own server is contacted. This is for
 *	   illustration purposes only. The service is not guaranteed and should
 *	   not be used more often than every ten minutes!
 *
 * @{
 */
/**
 * \file
 * TIME client implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct clock_sync_state
{
	int timer;
	int socket;
	clock_sync_callback callback;
};

static struct clock_sync_state state = { -1, -1, 0 };

static void clock_sync_interval(int timer);
static void clock_sync_connection_handler(int socket, enum tcp_event event);

/**
 * Starts the clock synchronization.
 *
 * \param[in] callback The callback function called for generated events. May be null.
 * \returns \c true if clock synchronization was started successfully, \c false on failure.
 */
bool clock_sync_start(clock_sync_callback callback)
{
	if(state.timer >= 0) {
		return false;
	}

	state.timer = timer_alloc(clock_sync_interval, 0);
	if(!timer_set(state.timer, 1)) {
		clock_sync_abort();
		return false;
	}

	state.callback = callback;

	return true;
}

/**
 * Aborts clock synchronization.
 */
void clock_sync_abort()
{
	timer_free(state.timer);
	state.timer = -1;
	
	tcp_socket_free(state.socket);
	state.socket = -1;
}

/**
 * Clock refresh interval timeout function.
 */
void clock_sync_interval(int timer)
{
	if(state.socket >= 0)
	{
		/* abort an existing connection, it is probably hung anyway */
		tcp_socket_free(state.socket);
		state.socket = -1;

		/* wait the configured interval */
		timer_set(timer, (uint32_t) CLOCK_SYNC_INTERVAL * 1000);

		if(state.callback)
			state.callback(CLOCK_SYNC_EVT_TIMEOUT);

		return;
	}

	/* start a new connection */
	uint8_t clock_sync_ip[4];
	clock_sync_ip[0] = CLOCK_SYNC_IP_0;
	clock_sync_ip[1] = CLOCK_SYNC_IP_1;
	clock_sync_ip[2] = CLOCK_SYNC_IP_2;
	clock_sync_ip[3] = CLOCK_SYNC_IP_3;

	state.socket = tcp_socket_alloc(clock_sync_connection_handler);
	tcp_connect(state.socket, clock_sync_ip, 37);

	/* start timeout */
	timer_set(timer, CLOCK_SYNC_TIMEOUT * 1000);
}

/**
 * Callback function which handles the synchronization connection.
 */
void clock_sync_connection_handler(int socket, enum tcp_event event)
{
	switch(event)
	{
		case TCP_EVT_CONN_CLOSED:
		{
			/* deallocate socket */
			tcp_socket_free(socket);
			state.socket = -1;

			/* wait the configured interval */
			timer_set(state.timer, (uint32_t) CLOCK_SYNC_INTERVAL * 1000);

			break;
		}
		case TCP_EVT_DATA_RECEIVED:
		{
			/* close connection */
			tcp_disconnect(socket);

			/* read time stamp */
			uint8_t clock_buffer[4];
			if(tcp_read(socket, clock_buffer, 4) != 4)
			{
				if(state.callback)
					state.callback(CLOCK_SYNC_EVT_ERROR);
				return;
			}

			/* interpret time stamp */
			uint32_t seconds = ((uint32_t) clock_buffer[0]) << 24 |
							   ((uint32_t) clock_buffer[1]) << 16 |
							   ((uint32_t) clock_buffer[2]) << 8 |
							   ((uint32_t) clock_buffer[3]) << 0;

			if(seconds < 2524521600 /* 1. Jan 1980 00:00 GMT */)
			{
				if(state.callback)
					state.callback(CLOCK_SYNC_EVT_ERROR);
				return;
			}

			/* set internal clock */
			seconds -= 2524521600;
			clock_set(seconds);

			/* notify application */
			if(state.callback)
				state.callback(CLOCK_SYNC_EVT_SUCCESS);

			break;
		}
		default:
		{
			break;
		}
	}
}

/**
 * @}
 * @}
 */

