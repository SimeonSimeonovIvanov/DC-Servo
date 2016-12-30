
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
#include "icmp.h"
#include "ip.h"
#include "net.h"
#include "tcp.h"
#include "udp.h"

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
 * \addtogroup net_stack_ip IP protocol support
 *
 * Implementation of the IP layer.
 *
 * @{
 */
/**
 * \file
 * IP implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

#define IP_FLAGS_DO_NOT_FRAGMENT 0x02
#define IP_FLAGS_MORE_FRAGMENTS 0x01

struct ip_header
{
    union
    {
        uint8_t version;
        uint8_t length_header;
    } vlh;
    uint8_t type_of_service;
    uint16_t length_packet;
    uint16_t identification;
    union
    {
        uint16_t flags;
        uint16_t fragment_offset;
    } ffo;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t source[4];
    uint8_t destination[4];
};

static uint8_t ip_address[4];
static uint8_t ip_netmask[4];
static uint8_t ip_gateway[4];
static uint8_t ip_broadcast[4];

static bool ip_is_directly_connected(const uint8_t* ip_remote);
static bool ip_is_broadcast(const uint8_t* ip_remote);

/**
 * Initializes the IP layer and optionally assigns IP address, netmask and gateway.
 *
 * \param[in] ip Buffer which contains an IP address. May be NULL.
 * \param[in] netmask Buffer which contains the netmask. May be NULL.
 * \param[in] gateway Buffer which contains the gateway. May be NULL.
 */
void ip_init(const uint8_t* ip, const uint8_t* netmask, const uint8_t* gateway)
{
    ip_set_address(ip);
    ip_set_netmask(netmask);
    ip_set_gateway(gateway);
}

/**
 * Forwards incoming packets to the appropriate higher-level protocol.
 *
 * \note This function is used internally and should not be explicitly called by applications.
 *
 * \param[in] packet The packet to be forwarded.
 * \param[in] packet_len The length of the packet.
 * \returns \c true if the packet has successfully been handled, \c false on failure.
 */
bool ip_handle_packet(const struct ip_header* packet, uint16_t packet_len)
{
    /* check protocol version */
    if((packet->vlh.version >> 4) != 0x04)
        return false;

    /* get header length */
    uint8_t header_length = (packet->vlh.length_header & 0x0f) * 4;
    /* get packet length */
    uint16_t packet_length = ntoh16(packet->length_packet);

    /* no support for fragmentation */
    if(NTOH16(packet->ffo.flags) & (IP_FLAGS_MORE_FRAGMENTS << 13) || /* flags */
       NTOH16(packet->ffo.fragment_offset) & 0x1fff                   /* fragment offset */
      )
        return false;

    /* check checksum */
    if(ntoh16(packet->checksum) != ~net_calc_checksum(0, (uint8_t*) packet, header_length, 10))
        return false;

    /* check packet size */
    if(packet_length > packet_len)
        return false;

    /* check destination address */
    if(memcmp(packet->destination, ip_address, sizeof(packet->destination)) != 0)
    {
        /* check if packet is a broadcast */
        if(packet->destination[0] != 0xff ||
           packet->destination[1] != 0xff ||
           packet->destination[2] != 0xff ||
           packet->destination[3] != 0xff 
          )
            return false;
    }

    /* hand packet over to higher-level protocols */
    switch(packet->protocol)
    {
        case IP_PROTOCOL_ICMP:
            return icmp_handle_packet(packet->source, (const struct icmp_header*) ((const uint8_t*) packet + header_length), packet_length - header_length);
        case IP_PROTOCOL_TCP:
            return tcp_handle_packet(packet->source, (const struct tcp_header*) ((const uint8_t*) packet + header_length), packet_length - header_length);
        case IP_PROTOCOL_UDP:
            return udp_handle_packet(packet->source, (const struct udp_header*) ((const uint8_t*) packet + header_length), packet_length - header_length);
        default:
            return false;
    }
}

/**
 * Sends data from the transmit buffer to a remote host.
 *
 * \param[in] ip_dest The IP address of the remote host to which the packet gets sent.
 * \param[in] protocol The protocol identifier of the upper layer protocol.
 * \param[in] data_len The length of the packet payload which is read from the transmit buffer.
 * \returns \c true on success, \c false on failure.
 */
bool ip_send_packet(const uint8_t* ip_dest, uint8_t protocol, uint16_t data_len)
{
    uint8_t mac_dest[6];
    /* check if destination ip is a broadcast */
    if(ip_is_broadcast(ip_dest))
    {
        memset(mac_dest, 255, sizeof(mac_dest));
    }
    else
    {
        /* check if destination ip is in our subnet */
        const uint8_t* ip_mac_target;
        if(ip_is_directly_connected(ip_dest))
        {
            ip_mac_target = ip_dest;
        }
        else
        {
            /* check if a valid gateway is configured */
            if((ip_gateway[0] | ip_gateway[1] | ip_gateway[2] | ip_gateway[3]) != 0)
                ip_mac_target = ip_gateway;
            else
                return false;
        }

        /* get mac address of destination via ARP */
        if(!arp_get_mac(ip_mac_target, mac_dest))
            /* The ARP layer currently does not have the
             * destination MAC address available, but an
             * ARP request has been sent now. We can not
             * deliver this packet until we get the ARP
             * response. We will return success, although
             * we lose the packet. The loss will have to
             * be handled by higher-level protocols.
             */
            return true;
    }
    
    struct ip_header* header = (struct ip_header*) ethernet_get_buffer();
    memset(header, 0, NET_HEADER_SIZE_IP);

    /* IPv4 */
    header->vlh.version |= 0x04 << 4;

    /* header size */
    header->vlh.length_header |= (NET_HEADER_SIZE_IP / 4) & 0x0f;

    /* packet size */
    header->length_packet = hton16(NET_HEADER_SIZE_IP + data_len);
    
    /* time to live */
    header->ttl = 64;

    /* set protocol */
    header->protocol = protocol;

    /* we are source */
    memcpy(header->source, ip_address, sizeof(header->source));

    /* set destination */
    memcpy(header->destination, ip_dest, sizeof(header->destination));

    /* checksum */
    header->checksum = hton16(~net_calc_checksum(0, (uint8_t*) header, NET_HEADER_SIZE_IP, 10));

    /* send packet */
    return ethernet_send_packet(mac_dest,
                                ETHERNET_FRAME_TYPE_IP,
                                NET_HEADER_SIZE_IP + data_len
                               );
}

/**
 * Retrieves the current IP address.
 *
 * \returns The buffer containing the IP address.
 */
const uint8_t* ip_get_address()
{
    return ip_address;
}

/**
 * Configures a new IP address.
 *
 * \param[in] ip_local The buffer containing the new IP address. May be NULL to accept broadcasts only.
 */
void ip_set_address(const uint8_t* ip_local)
{
    memset(ip_address, 0, 4);

    if(ip_local)
        memcpy(ip_address, ip_local, 4);
}

/**
 * Retrieves the current IP netmask.
 *
 * \returns The buffer containing the IP netmask.
 */
const uint8_t* ip_get_netmask()
{
    return ip_netmask;
}

/**
 * Configures a new IP netmask.
 *
 * \param[in] netmask The buffer containing the new IP netmask.
 */
void ip_set_netmask(const uint8_t* netmask)
{
    memset(ip_netmask, 255, 4);

    if(netmask)
        memcpy(ip_netmask, netmask, 4);
    else
        ip_netmask[3] = 0;
}

/**
 * Retrieves the current IP gateway.
 *
 * \returns The buffer containing the IP gateway.
 */
const uint8_t* ip_get_gateway()
{
    return ip_gateway;
}

/**
 * Configures a new IP gateway.
 *
 * \param[in] gateway The buffer containing the new IP gateway.
 */
void ip_set_gateway(const uint8_t* gateway)
{
    memset(ip_gateway, 0, 4);

    if(gateway)
        memcpy(ip_gateway, gateway, 4);
}

/**
 * Retrieves the broadcast address.
 *
 * \returns The buffer containing the broadcast address.
 */
const uint8_t* ip_get_broadcast()
{
    uint8_t zero = 0;
    for(uint8_t i = 0; i < sizeof(ip_broadcast); ++i)
    {
        ip_broadcast[i] = ip_address[i] | ~ip_netmask[i];
        zero |= ip_broadcast[i];
    }

    if(zero == 0)
        memset(ip_broadcast, 255, sizeof(ip_broadcast));

    return ip_broadcast;
}

/**
 * Determines wether a remote host is part of the local subnet or not.
 *
 * \param[in] ip_remote The IP address of the remote host.
 * \returns \c true if the given host belongs to the local subnet, \c false otherwise.
 */
bool ip_is_directly_connected(const uint8_t* ip_remote)
{
    for(uint8_t i = 0; i < 4; ++i)
    {
        if((ip_remote[i] & ip_netmask[i]) != (ip_address[i] & ip_netmask[i]))
            return false;
    }

    return true;
}

/**
 * Determines wether an IP address is a broadcast address or not.
 *
 * \returns \c true if the given address is a broadcast, \c false otherwise.
 */
bool ip_is_broadcast(const uint8_t* ip_remote)
{
    return (*((uint32_t*) ip_remote) == HTON32(0xffffffff)) ||
           memcmp(ip_remote, ip_get_broadcast(), 4) == 0;
}

/**
 * @}
 * @}
 * @}
 */

