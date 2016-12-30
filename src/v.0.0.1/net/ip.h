
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IP_H
#define IP_H

#include <stdbool.h>
#include <stdint.h>

#include "ethernet.h"
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
 * \addtogroup net_stack_ip
 *
 * @{
 */
/**
 * \file
 * IP header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/** Protocol identifier of the ICMP protocol. */
#define IP_PROTOCOL_ICMP 0x01
/** Protocol identifier of the TCP protocol. */
#define IP_PROTOCOL_TCP 0x06
/** Protocol identifier of the UDP protocol. */
#define IP_PROTOCOL_UDP 0x11

struct ip_header;

void ip_init(const uint8_t* ip, const uint8_t* netmask, const uint8_t* gateway);

bool ip_handle_packet(const struct ip_header* packet, uint16_t packet_len);

bool ip_send_packet(const uint8_t* ip_dest, uint8_t protocol, uint16_t data_len);

const uint8_t* ip_get_address();
void ip_set_address(const uint8_t* ip_local);

const uint8_t* ip_get_netmask();
void ip_set_netmask(const uint8_t* netmask);

const uint8_t* ip_get_gateway();
void ip_set_gateway(const uint8_t* gateway);

const uint8_t* ip_get_broadcast();

/**
 * Retrieves a pointer to the IP transmit buffer.
 *
 * For sending IP packets, write your data to the buffer returned by this
 * function and call ip_send_packet() afterwards.
 *
 * \returns A pointer pointing to the beginning of the transmit buffer.
 */
#define ip_get_buffer() (ethernet_get_buffer() + NET_HEADER_SIZE_IP)
/**
 * Retrieves the maximum payload size of the transmit buffer.
 *
 * You may not send more bytes per IP packet than returned by this function.
 *
 * \returns The maximum payload size per IP packet.
 */
#define ip_get_buffer_size() (ethernet_get_buffer_size() - NET_HEADER_SIZE_IP)

/**
 * @}
 * @}
 * @}
 */

#endif

