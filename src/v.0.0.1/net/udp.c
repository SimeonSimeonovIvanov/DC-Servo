
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ip.h"
#include "net.h"
#include "udp.h"

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
 * \addtogroup net_stack_udp UDP protocol support
 *
 * Implementation of the UDP protocol.
 *
 * To receive UDP packets, a socket has to be allocated using
 * udp_socket_alloc() and bound to a local port with udp_bind_local().
 * For each packet received, the socket callback function gets called.
 *
 * For being able to send UDP packets, an additional call to
 * udp_bind_remote() is needed. Binding a socket to a
 * remote host has two consequences:
 * -# Incoming packets on this socket are forwarded only if they
 *    match the remote binding.
 * -# Transmitted packets are sent to the remote host and port
 *    of the remote binding.
 *
 * @{
 */
/**
 * \file
 * UDP implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct udp_socket
{
    udp_callback callback;
    uint16_t port_local;
    uint16_t port_remote;
    uint8_t ip_remote[4];
};

struct udp_header
{
    uint16_t port_source;
    uint16_t port_destination;
    uint16_t length;
    uint16_t checksum;
};

static struct udp_socket udp_sockets[UDP_MAX_SOCKET_COUNT];

#define FOREACH_SOCKET(socket) \
    for((socket) = &udp_sockets[0]; (socket) < &udp_sockets[UDP_MAX_SOCKET_COUNT]; ++(socket))

static int udp_socket_number(const struct udp_socket* socket);

static bool udp_port_is_used(uint16_t port);
static uint16_t udp_port_find_unused();
static uint16_t udp_calc_checksum(const uint8_t* ip_destination, const struct udp_header* packet, uint16_t packet_len);

/**
 * Initializes the UDP layer. This function has to be called once on startup.
 */
void udp_init()
{
    memset(udp_sockets, 0, sizeof(udp_sockets));
}

/**
 * Forwards incoming packets to the appropriate UDP socket.
 *
 * \note This function is used internally and should not be explicitly called by applications.
 *
 * \returns \c true if a matching socket was found, \c false if the packet was discarded.
 */
bool udp_handle_packet(const uint8_t* ip_remote, const struct udp_header* packet, uint16_t packet_len)
{
    if(packet_len < sizeof(*packet))
        return false;

    /* test checksum */
    if(udp_calc_checksum(ip_remote, packet, packet_len) != ntoh16(packet->checksum))
        /* invalid checksum */
        return false;

    /* check for valid port numbers */
    uint16_t packet_port_local = ntoh16(packet->port_destination);
    uint16_t packet_port_remote = ntoh16(packet->port_source);
    if(packet_port_local == 0 || packet_port_remote == 0)
        return false;

    /* search socket this packet should get forwarded to */
    struct udp_socket* socket;
    FOREACH_SOCKET(socket)
    {
        if(socket->port_local != packet_port_local)
            continue;
        if(socket->port_remote != 0 && socket->port_remote != packet_port_remote)
            continue;
        if((socket->ip_remote[0] | socket->ip_remote[1] | socket->ip_remote[2] | socket->ip_remote[3]) != 0x00 &&
           memcmp(socket->ip_remote, ip_remote, sizeof(socket->ip_remote)) != 0
          )
            continue;

        int socket_number = udp_socket_number(socket);

        udp_bind_remote(socket_number, ip_remote, packet_port_remote);
        socket->callback(socket_number, (uint8_t*) (packet + 1), packet_len);
        return true;
    }

    return false;
}

/**
 * Allocates a new UDP socket.
 *
 * Each socket has an attached callback function which gets called for every
 * packet which arrives on the socket.
 *
 * A socket is identified by a non-negative integer. Use udp_socket_valid()
 * to check if the returned socket is valid.
 *
 * \param[in] callback Pointer to the callback function which gets attached to the new socket.
 * \returns The non-negative socket identifier on success, \c -1 on failure.
 */
int udp_socket_alloc(udp_callback callback)
{
    if(!callback)
        return -1;

    /* search free socket structure */
    struct udp_socket* socket;
    FOREACH_SOCKET(socket)
    {
        if(socket->port_local == 0)
        {
            /* allocate socket */
            memset(socket, 0, sizeof(*socket));
            socket->port_local = udp_port_find_unused();
            socket->callback = callback;
            return udp_socket_number(socket);
        }
    }

    return -1;
}

/**
 * Deallocates an UDP socket and makes it available for future allocations.
 *
 * After freeing the socket, it can no longer be used.
 *
 * \param[in] socket The identifier of the socket which is to be deallocated.
 * \returns \c true if the socket has been deallocated, \c false on failure.
 */
bool udp_socket_free(int socket)
{
    if(!udp_socket_valid(socket))
        return false;

    /* mark socket as free */
    udp_sockets[socket].port_local = 0;
    return true;
}

/**
 * Bind an UDP socket to a local port.
 *
 * Specifies which port the incoming packet must be addressed
 * to in order to be received by the socket.
 *
 * \param[in] socket The identifier of the socket which is to be locally bound.
 * \param[in] port_local The port which should be bound to the socket.
 * \returns \c true if the binding could be established, \c false otherwise.
 */
bool udp_bind_local(int socket, uint16_t port_local)
{
    if(!udp_socket_valid(socket) || port_local < 1)
        return false;

    /* return success when we already listen on the same port */
    struct udp_socket* sck = &udp_sockets[socket];
    if(sck->port_local == port_local)
        return true;

    /* make sure listening port is not already active */
    if(udp_port_is_used(port_local))
        return false;

    /* change socket to listen on the given port number */
    udp_sockets[socket].port_local = port_local;
    return true;
}

/**
 * Bind an UDP socket to a remote port and optionally to a remote host.
 *
 * Binding an UDP socket to a remote host and/or port
 * has two consequences:
 * -# All packets sent via this socket are sent to this remote host.
 * -# Incoming packets are only forwarded to the application if their
 *    source ip and/or port matches the values the socket is bound to.
 *
 * A remote ip of NULL means "broadcast"/"accept all".
 *
 * \param[in] socket The identifier of the socket which is to be remotely bound.
 * \param[in] ip_remote The remote IP address of the host to which to bind the socket, or NULL.
 * \param[in] port_remote The remote port to which to bind the socket.
 * \returns \c true if the binding could be established, \c false otherwise.
 */
bool udp_bind_remote(int socket, const uint8_t* ip_remote, uint16_t port_remote)
{
    if(!udp_socket_valid(socket))
        return false;

    struct udp_socket* sck = &udp_sockets[socket];
    memset(sck->ip_remote, 0, sizeof(sck->ip_remote));

    sck->port_remote = port_remote;
    if(ip_remote)
        memcpy(sck->ip_remote, ip_remote, sizeof(sck->ip_remote));

    return true;
}

/**
 * Remove the remote binding of an UDP socket.
 *
 * Note that after the socket has been remotely unbound, it receives
 * packets from any remote host which are sent to the socket's local port.
 *
 * \param[in] socket The identifier of the socket which is to be remotely unbound.
 * \returns \c true if the binding could be removed, \c false otherwise.
 */
bool udp_unbind_remote(int socket)
{
    if(!udp_socket_valid(socket))
        return false;

    /* unbind socket from remote ip and port */
    struct udp_socket* sck = &udp_sockets[socket];
    memset(sck->ip_remote, 0, sizeof(sck->ip_remote));
    sck->port_remote = 0;

    return true;
}

/**
 * Send an UDP packet from the transmit buffer to a remote host.
 *
 * Note that before using this function, the socket must have been locally
 * and remotely bound via udp_socket_bind_local() and udp_socket_bind_remote().
 *
 * \param[in] socket The identifier of the socket via which to send the packet.
 * \param[in] data_len The payload length of the packet.
 * \returns \c true if the packet was successfully sent, \c false on failure.
 */
bool udp_send(int socket, uint16_t data_len)
{
    if(!udp_socket_valid(socket))
        return -1;

    /* get socket descriptor */
    struct udp_socket* sck = &udp_sockets[socket];

    /* check if remote target port is valid */
    if(sck->port_remote < 1)
        return -1;

    /* check if we send a broadcast */
    const uint8_t* ip_remote = sck->ip_remote;
    if((ip_remote[0] | ip_remote[1] | ip_remote[2] | ip_remote[3]) == 0x00)
        ip_remote = ip_get_broadcast();

    /* check maximum packet size */
    struct udp_header* header = (struct udp_header*) ip_get_buffer();
    uint16_t packet_data_max = ip_get_buffer_size() - sizeof(*header);
    if(data_len > packet_data_max)
        data_len = packet_data_max;

    /* prepare udp header */
    memset(header, 0, sizeof(*header));
    header->port_source = hton16(sck->port_local);
    header->port_destination = hton16(sck->port_remote);
    header->length = hton16(sizeof(*header) + data_len);
    header->checksum = hton16(udp_calc_checksum(ip_remote, header, sizeof(*header) + data_len));
    
    /* send packet via the ip layer */
    return ip_send_packet(ip_remote,
                          IP_PROTOCOL_UDP,
                          sizeof(*header) + data_len
                         );
}

/**
 * Retrieves the socket number identifying the internal socket structure.
 *
 * \param[in] socket The internal socket structure for which to find the identifier.
 * \returns The non-negative socket identifier on success, -1 on failure.
 */
int udp_socket_number(const struct udp_socket* socket)
{
    if(!socket)
        return -1;

    int sck = ((int) socket - (int) &udp_sockets[0]) / sizeof(*socket);
    if(!udp_socket_valid(sck))
        return -1;

    return sck;
}

/**
 * Checks if the specified port is already locally bound to a socket.
 *
 * \param[in] port The port number which should be checked.
 * \returns \c true if the port is already bound, \c false otherwise.
 */
bool udp_port_is_used(uint16_t port)
{
    if(port < 1)
        return true;

    struct udp_socket* socket;
    FOREACH_SOCKET(socket)
    {
        if(socket->port_local == port)
            return true;
    }

    return false;
}

/**
 * Searches for an unbound, unpriviledged local port number.
 *
 * \returns An unbound port.
 */
uint16_t udp_port_find_unused()
{
    static uint16_t port_counter = 1023;
    
    do
    {
        if(++port_counter < 1024)
            port_counter = 1024;

    } while(udp_port_is_used(port_counter));

    return port_counter;
}

/**
 * Calculates the checksum of a UDP packet.
 *
 * \returns The 16-bit checksum.
 */
uint16_t udp_calc_checksum(const uint8_t* ip_destination, const struct udp_header* packet, uint16_t packet_len)
{
    /* pseudo header */
    uint16_t checksum = IP_PROTOCOL_UDP + packet_len;
    checksum = net_calc_checksum(checksum, ip_get_address(), 4, 4);
    checksum = net_calc_checksum(checksum, ip_destination, 4, 4);

    /* real package */
    return ~net_calc_checksum(checksum, (uint8_t*) packet, packet_len, 6);
}

/**
 * @}
 * @}
 * @}
 */

