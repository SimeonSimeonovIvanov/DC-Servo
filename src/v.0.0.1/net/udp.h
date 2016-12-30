
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef UDP_H
#define UDP_H

#include <stdbool.h>
#include <stdint.h>

#include "ip.h"
#include "net.h"
#include "udp_config.h"

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
 * \file
 * UDP header (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct udp_header;

typedef void (*udp_callback)(int socket, uint8_t* data, uint16_t data_len);

void udp_init();

bool udp_handle_packet(const uint8_t* ip_remote, const struct udp_header* packet, uint16_t packet_len);

int udp_socket_alloc(udp_callback callback);
bool udp_socket_free(int socket);

bool udp_bind_local(int socket, uint16_t port_local);
bool udp_bind_remote(int socket, const uint8_t* ip_remote, uint16_t port_remote);
bool udp_unbind_remote(int socket);

bool udp_send(int socket, uint16_t data_len);

/**
 * Checks if the given number is a valid socket identifier.
 *
 * \param[in] socket The socket identifier to check.
 * \returns TRUE if the argument is a valid socket number, FALSE otherwise.
 */
#define udp_socket_valid(socket) (((unsigned int) (socket)) < UDP_MAX_SOCKET_COUNT)

/**
 * Retrieves a pointer to the UDP transmit buffer.
 *
 * For sending UDP packets, write your data to the buffer returned by this
 * function and call udp_send_packet() afterwards.
 *
 * \returns A pointer pointing to the beginning of the transmit buffer.
 */
#define udp_get_buffer() (ip_get_buffer() + NET_HEADER_SIZE_UDP)
/**
 * Retrieves the maximum payload size of the transmit buffer.
 *
 * You may not send more bytes per UDP packet than returned by this function.
 *
 * \returns The maximum payload size per UDP packet.
 */
#define udp_get_buffer_size() (ip_get_buffer_size() - NET_HEADER_SIZE_UDP)

/**
 * @}
 * @}
 * @}
 */

#endif

