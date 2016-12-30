
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ETHERNET_H
#define ETHERNET_H

#include "ethernet_config.h"
#include "net.h"

#include <stdbool.h>
#include <stdint.h>

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
 * \addtogroup net_stack_ethernet
 *
 * @{
 */
/**
 * \file
 * Ethernet header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/** Protocol identifier of the IP protocol. */
#define ETHERNET_FRAME_TYPE_IP 0x0800
/** Protocol identifier of the ARP protocol. */
#define ETHERNET_FRAME_TYPE_ARP 0x0806

extern uint8_t ethernet_tx_buffer[];

bool ethernet_handle_packet();
void ethernet_init(const uint8_t* mac);
bool ethernet_send_packet(const uint8_t* mac_dest, uint16_t type, uint16_t data_len);

const uint8_t* ethernet_get_mac();

/**
 * Retrieves a pointer to the ethernet transmit buffer.
 *
 * For sending ethernet packets, write your data to the buffer returned by this
 * function and call ethernet_send_packet() afterwards.
 *
 * \returns A pointer pointing to the beginning of the transmit buffer.
 */
#define ethernet_get_buffer() (&ethernet_tx_buffer[0] + NET_HEADER_SIZE_ETHERNET)
/**
 * Retrieves the maximum payload size of the transmit buffer.
 *
 * You may not send more bytes per ethernet packet than returned by this function.
 *
 * \returns The maximum payload size per ethernet packet.
 */
#define ethernet_get_buffer_size() (ETHERNET_MAX_PACKET_SIZE)

/**
 * @}
 * @}
 * @}
 */

#endif

