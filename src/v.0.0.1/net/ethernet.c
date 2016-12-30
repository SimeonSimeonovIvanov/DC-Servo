
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>

#include "arp.h"
#include "ethernet.h"
#include "ethernet_config.h"
#include "hal.h"
#include "ip.h"
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
 * \addtogroup net_stack_ethernet Ethernet frame support
 *
 * Implementation of the Ethernet protocol.
 *
 * @{
 */
/**
 * \file
 * Ethernet implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct ethernet_header
{
    uint8_t destination[6];
    uint8_t source[6];
    uint16_t type_length;
};

/* global packet send and receive buffers */
uint8_t ethernet_rx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET];
uint8_t ethernet_tx_buffer[ETHERNET_MAX_PACKET_SIZE + NET_HEADER_SIZE_ETHERNET];

/* our configured ethernet address */
static uint8_t ethernet_mac[6];

/**
 * Initializes the ethernet layer and assigns an ethernet address.
 *
 * \param[in] mac The ethernet MAC address to configure for the network interface. Six bytes are used from this buffer.
 */
void ethernet_init(const uint8_t* mac)
{
	memset(ethernet_mac, 0, sizeof(ethernet_mac));

	if(NULL != mac) {
		memcpy(ethernet_mac, mac, sizeof(ethernet_mac));
	}
}

/**
 * Reads incoming packets and forwards them to the appropriate higher-level protocol.
 *
 * \note This function must be continuously called by the application within its main loop
 *       in order to properly handle incoming packets.
 *
 * \returns \c true if a packet has been handled, \c false if none has been waiting.
 */
bool ethernet_handle_packet()
{
    uint16_t packet_size = hal_receive_packet(ethernet_rx_buffer, sizeof(ethernet_rx_buffer));
    if(packet_size < 1)
        return false;

    struct ethernet_header* header = (struct ethernet_header*) ethernet_rx_buffer;
    uint8_t* data = (uint8_t*) (header + 1);
    packet_size -= NET_HEADER_SIZE_ETHERNET; /* drop ethernet header */

    switch(header->type_length)
    {
        case HTON16(ETHERNET_FRAME_TYPE_ARP):
            arp_handle_packet((struct arp_header*) data, packet_size);
            break;
        case HTON16(ETHERNET_FRAME_TYPE_IP):
            ip_handle_packet((struct ip_header*) data, packet_size);
            break;
        default:
            break;
    }

    return true;
}

/**
 * Sends data from the transmit buffer to a remote host.
 *
 * \param[in] mac_dest The six byte ethernet address of the remote host to which the packet gets sent.
 * \param[in] type The protocol identifier of the upper layer protocol.
 * \param[in] data_len The length of the packet payload which is read from the global transmit buffer.
 * \returns \c true on success, \c false on failure.
 */
bool ethernet_send_packet(const uint8_t* mac_dest, uint16_t type, uint16_t data_len)
{
    if(data_len > sizeof(ethernet_rx_buffer) - NET_HEADER_SIZE_ETHERNET)
        return false;

    struct ethernet_header* header = (struct ethernet_header*) ethernet_tx_buffer;
    memcpy(header->destination, mac_dest, 6);
    memcpy(header->source, ethernet_mac, 6);
    header->type_length = hton16(type);

    return hal_send_packet((uint8_t*) header, data_len + NET_HEADER_SIZE_ETHERNET);
}

/**
 * Retrieves the current ethernet address.
 *
 * \returns The buffer containing the ethernet address.
 */
const uint8_t* ethernet_get_mac()
{
    return ethernet_mac;
}

/**
 * @}
 * @}
 * @}
 */

