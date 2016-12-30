
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "icmp.h"
#include "ip.h"
#include "net.h"

#include <string.h>

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
 * \addtogroup net_stack_icmp ICMP protocol support
 *
 * Implementation of the ICMP layer.
 *
 * \note This is kept very simple. It basically just answers ping requests.
 *
 * @{
 */
/**
 * \file
 * ICMP implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

#define ICMP_TYPE_ECHO_REPLY 0x00
#define ICMP_TYPE_DESTINATION_UNREACHABLE 0x03
#define ICMP_TYPE_ECHO_REQUEST 0x08

struct icmp_header
{
	uint8_t type;
	uint8_t class;
	uint16_t checksum;
};

/**
 * Initializes the ICMP layer.
 */
void icmp_init()
{
}

/**
 * Handles ICMP packets.
 *
 * Currently, this function just answers ping requests.
 *
 * \note This function is used internally and should not be explicitly called by applications.
 *
 * \param[in] ip The IP address of the host where the ICMP packet came from.
 * \param[in] packet A pointer to the buffer containing the ICMP packet.
 * \param[in] packet_len The length of the ICMP packet.
 * \returns \c true if the incoming packet could be handled, \c false otherwise.
 */
bool icmp_handle_packet(const uint8_t* ip, const struct icmp_header* packet, uint16_t packet_len)
{
	if(ntoh16(packet->checksum) != ~net_calc_checksum(0, (const uint8_t*) packet, packet_len, 2))
		return false;

	switch(packet->type)
	{
		case ICMP_TYPE_ECHO_REQUEST:
		{
			/* create packet as a copy */
			struct icmp_header* reply = (struct icmp_header*) ip_get_buffer();
			memcpy(reply, packet, packet_len);

			/* we want to reply */
			reply->type = ICMP_TYPE_ECHO_REPLY;

			/* calculate new checksum */
			reply->checksum = hton16(~net_calc_checksum(0, (const uint8_t*) reply, packet_len, 2));

			return ip_send_packet(ip, IP_PROTOCOL_ICMP, packet_len);
		}   
		case ICMP_TYPE_ECHO_REPLY:
		case ICMP_TYPE_DESTINATION_UNREACHABLE:
			/* TODO */
		default:
			return false;
	}
}
