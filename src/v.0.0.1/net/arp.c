
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>

#include "../sys/timer.h"

#include "arp.h"
#include "arp_config.h"
#include "ethernet.h"
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
 * \addtogroup net_stack_arp ARP protocol support
 *
 * Implementation of the ARP layer.
 *
 * @{
 */
/**
 * \file
 * ARP implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

#define ARP_HW_ADDR_TYPE_ETHERNET 0x0001
#define ARP_HW_ADDR_SIZE_ETHERNET 0x06
#define ARP_PROT_ADDR_TYPE_IP 0x0800
#define ARP_PROT_ADDR_SIZE_IP 0x04
#define ARP_OP_REQUEST 0x0001
#define ARP_OP_REPLY 0x0002

struct arp_header
{
    uint16_t hardware_address_type;
    uint16_t protocol_address_type;
    uint8_t hardware_address_size;
    uint8_t protocol_address_size;
    uint16_t operation;
    uint8_t source_mac[6];
    uint8_t source_ip[4];
    uint8_t dest_mac[6];
    uint8_t dest_ip[4];
};

struct arp_entry
{
    uint8_t timeout;
    uint8_t mac[6];
    uint8_t ip[4];
};

static void arp_interval(int timer);
static bool arp_generate_reply(const struct arp_header* packet);
static bool arp_generate_request(const uint8_t* ip);
static void arp_insert(const uint8_t* mac, const uint8_t* ip);

static struct arp_entry arp_table[ARP_TABLE_SIZE];

/**
 * Initializes the ARP layer.
 */
void arp_init()
{
    memset(arp_table, 0, sizeof(arp_table));

    int timer = timer_alloc(arp_interval, 0);
    timer_set(timer, 5000);
}

/**
 * Global ARP timeout handler.
 *
 * \param[in] timer The identifier of the timer which generated the timeout.
 */
void arp_interval(int timer)
{
    struct arp_entry* entry = &arp_table[0];
    for(; entry < &arp_table[ARP_TABLE_SIZE]; ++entry)
    {
        if(entry->timeout > 0)
            --entry->timeout;
    }

    timer_set(timer, 5000);
}

/**
 * \internal
 * Global ARP packet handler for incoming ARP packets.
 *
 * \param[in] packet A pointer to the buffer containing the incoming ARP packet.
 * \param[in] packet_len The length of the incoming packet.
 * \returns \c true if the packet was successfully handled, \c false otherwise.
 */
bool arp_handle_packet(const struct arp_header* packet, uint16_t packet_len)
{
    if(packet_len < sizeof(*packet))
        return false;

    /* check if packet is for our ip address */
    if(memcmp(&packet->dest_ip, ip_get_address(), sizeof(packet->dest_ip)) != 0)
        return false;
    /* check hardware address type: ethernet */
    if(packet->hardware_address_type != HTON16(ARP_HW_ADDR_TYPE_ETHERNET))
        return false;
    /* check protocol address type: ip */
    if(packet->protocol_address_type != HTON16(ARP_PROT_ADDR_TYPE_IP))
        return false;
    /* check hardware and protocol address size: 6 bytes mac, 4 bytes ip */
    if(packet->hardware_address_size != ARP_HW_ADDR_SIZE_ETHERNET ||
       packet->protocol_address_size != ARP_PROT_ADDR_SIZE_IP)
        return false;

    /* check arp operation */
    uint16_t op = packet->operation;
    switch(op)
    {
        case HTON16(ARP_OP_REQUEST):
        case HTON16(ARP_OP_REPLY):

            /* put host into table */
            arp_insert(packet->source_mac, packet->source_ip);
            /* reply */
            return op == HTON16(ARP_OP_REQUEST) ? arp_generate_reply(packet) : true;

        default:
            return false;
    }
}

/**
 * Generates an ARP reply for a specific request.
 *
 * \param[in] packet A pointer to the buffer containing the ARP request to answer.
 * \returns \c true if the response was successfully generated and sent, \c false otherwise.
 */
bool arp_generate_reply(const struct arp_header* packet)
{
    struct arp_header* arp_reply = (struct arp_header*) ethernet_get_buffer();

    /* keep hardware types, protocol types and address sizes */
    arp_reply->hardware_address_type = packet->hardware_address_type;
    arp_reply->protocol_address_type = packet->protocol_address_type;
    arp_reply->hardware_address_size = packet->hardware_address_size;
    arp_reply->protocol_address_size = packet->protocol_address_size;
    
    /* we want to reply */
    arp_reply->operation = HTON16(ARP_OP_REPLY);

    /* source mac address */
    memcpy(arp_reply->source_mac, ethernet_get_mac(), sizeof(arp_reply->source_mac));
    /* source ip address */
    memcpy(arp_reply->source_ip, ip_get_address(), sizeof(arp_reply->source_ip));
    /* source mac and ip addresses of request are destination addresses of reply */
    memcpy(arp_reply->dest_mac, packet->source_mac, sizeof(arp_reply->dest_mac));
    memcpy(arp_reply->dest_ip, packet->source_ip, sizeof(arp_reply->dest_ip));

    return ethernet_send_packet(arp_reply->dest_mac,
                                ETHERNET_FRAME_TYPE_ARP,
                                sizeof(*arp_reply)
                               );
}

/**
 * Generates an ARP request asking for the hardware address of a specific IP address.
 *
 * \param[in] ip The IP address for which to request the hardware address.
 * \returns \c true if the request was successfully generated and sent, \c false otherwise.
 */
bool arp_generate_request(const uint8_t* ip)
{
    struct arp_header* arp_request = (struct arp_header*) ethernet_get_buffer();

    /* arp request for ipv4 over ethernet */
    arp_request->hardware_address_type = HTON16(ARP_HW_ADDR_TYPE_ETHERNET);
    arp_request->protocol_address_type = HTON16(ARP_PROT_ADDR_TYPE_IP);
    arp_request->hardware_address_size = ARP_HW_ADDR_SIZE_ETHERNET;
    arp_request->protocol_address_size = ARP_PROT_ADDR_SIZE_IP;

    /* request */
    arp_request->operation = HTON16(ARP_OP_REQUEST);

    /* we are source */
    memcpy(arp_request->source_mac, ethernet_get_mac(), sizeof(arp_request->source_mac));
    memcpy(arp_request->source_ip, ip_get_address(), sizeof(arp_request->source_ip));

    /* unknown destination mac address */
    memset(arp_request->dest_mac, 0, sizeof(arp_request->dest_mac));
    /* the ipv4 address we request */
    memcpy(arp_request->dest_ip, ip, sizeof(arp_request->dest_ip));

    /* send as a broadcast */
    uint8_t dest_mac[6];
    memset(dest_mac, 0xff, sizeof(dest_mac));
    return ethernet_send_packet(dest_mac,
                                ETHERNET_FRAME_TYPE_ARP,
                                sizeof(*arp_request)
                               );
}

/**
 * Inserts a host into the ARP table.
 *
 * \param[in] mac A pointer to the 6-byte hardware address to put into the table.
 * \param[in] ip A pointer to the 4-byte IP address to put into the table.
 */
void arp_insert(const uint8_t* mac, const uint8_t* ip)
{
    /* Search for a free entry. If none is
     * available, overwrite the oldest one.
     */
    struct arp_entry* entry = &arp_table[0];
    struct arp_entry* entry_used = entry;
    uint16_t timeout_min = 0xffff;
    for(; entry < &arp_table[ARP_TABLE_SIZE]; ++entry)
    {
        if(memcmp(entry->ip, ip, sizeof(entry->ip)) == 0)
        {
            entry_used = entry;
            break;
        }
        else if(timeout_min > entry->timeout)
        {
            timeout_min = entry->timeout;
            entry_used = entry;
        }
    }

    /* insert mac address */
    memcpy(entry_used->mac, mac, sizeof(entry_used->mac));
    /* insert ip address */
    memcpy(entry_used->ip, ip, sizeof(entry_used->ip));
    /* start timeout */
    entry_used->timeout = ARP_TIMEOUT;
}

/**
 * Retrieves the hardware address of a host by its IP address.
 *
 * \param[in] ip A pointer to the IP address of the host.
 * \param[out] mac A pointer to the buffer into which the host's hardware address is written.
 * \returns \c true if the host was found, \c false otherwise.
 */
bool arp_get_mac(const uint8_t* ip, uint8_t* mac)
{
    /* search for ip address in arp table */
    struct arp_entry* entry = &arp_table[0];
    for(; entry < &arp_table[ARP_TABLE_SIZE]; ++entry)
    {
        if(entry->timeout > 0 &&
           memcmp(entry->ip, ip, sizeof(entry->ip)) == 0
          )
        {
            memcpy(mac, entry->mac, sizeof(entry->mac));
            return true;
        }
    }

    /* send ARP request */
    arp_generate_request(ip);

    return false;
}

/**
 * @}
 * @}
 * @}
 */

