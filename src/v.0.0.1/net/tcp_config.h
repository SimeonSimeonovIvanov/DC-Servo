
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TCP_CONFIG_H
#define TCP_CONFIG_H

#include "ethernet_config.h"
#include "net.h"

/**
 * \addtogroup net
 *
 * @{
 */
/**
 * \addtogroup net_stack
 *
 * @{
 */
/**
 * \addtogroup net_stack_tcp
 *
 * @{
 */
/**
 * \addtogroup net_stack_tcp_config TCP configuration
 *
 * @{
 */
/**
 * \file
 * TCP configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * The maximum number of TCP sockets allocated in parallel.
 */
#define TCP_MAX_SOCKET_COUNT		25

/**
 * The maximum number of TCP sockets which may be associated with an active connection.
 *
 * Basically, this is TCP_MAX_SOCKET_COUNT without the listening connections.
 */
#define TCP_MAX_CONNECTION_COUNT	5 // ???

/**
 * The maximum segment size of outgoing TCP packets.
 */
#define TCP_MSS (ETHERNET_MAX_PACKET_SIZE - NET_HEADER_SIZE_IP - NET_HEADER_SIZE_TCP)

/**
 * The receive buffer size per connection.
 */
#define TCP_RECEIVE_BUFFER_SIZE TCP_MSS
/**
 * The transmit buffer size per connection.
 */
#define TCP_TRANSMIT_BUFFER_SIZE TCP_MSS

/**
 * The default round-trip-time estimation in seconds.
 */
#define TCP_TIMEOUT_RTT				1
/**
 * Timeout in seconds a connection must be idle before generating an event.
 */
#define TCP_TIMEOUT_IDLE			1
/**
 * Timeout in seconds used during connection initialization and shutdown.
 */
#define TCP_TIMEOUT_GENERIC			5
/**
 * Timeout after which the connection goes to the closed state.
 */
#define TCP_TIMEOUT_TIME_WAIT		40

/**
 * Maximum retransmission count.
 */
#define TCP_MAX_RETRY				5

/**
 * @}
 * @}
 * @}
 * @}
 */

#if TCP_MAX_SOCKET_COUNT < TCP_MAX_CONNECTION_COUNT
#undef TCP_MAX_SOCKET_COUNT
#define TCP_MAX_SOCKET_COUNT TCP_MAX_CONNECTION_COUNT
#endif

#endif

