
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef UDP_CONFIG_H
#define UDP_CONFIG_H

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
 * \addtogroup net_stack_udp
 *
 * @{
 */
/**
 * \addtogroup net_stack_udp_config UDP configuration
 *
 * @{
 */
/**
 * \file
 * UDP configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * The maximum number of UDP sockets allocated in parallel.
 */
#define UDP_MAX_SOCKET_COUNT 2

/**
 * The maximum segment size of outgoing UDP packets.
 */
#define UDP_MSS (ETHERNET_MAX_PACKET_SIZE - NET_HEADER_SIZE_IP - NET_HEADER_SIZE_UDP)

/**
 * @}
 * @}
 * @}
 * @}
 */

#endif

