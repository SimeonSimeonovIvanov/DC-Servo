
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ip.h"
#include "net.h"
#include "tcp.h"
#include "tcp_queue.h"

#include "../sys/timer.h"

#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#define TCP_DEBUG 0

#if TCP_DEBUG
#include <stdio.h>
#endif

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
 * \addtogroup net_stack_tcp TCP protocol support
 *
 * Implementation of the TCP protocol.
 *
 * For each listening port and each connection there exists a TCP socket.
 * Sockets are allocated using tcp_socket_alloc() and deallocated with
 * tcp_socket_free(). Each socket has a callback function associated
 * with it. This function is called whenever an event happens on the socket.
 * The callback function then acts on this event.
 *
 * To listen for incoming connections on a specific port, call tcp_listen()
 * on a socket. When a remote host wants to establish a connection, an event
 * is generated and tcp_accept() should be called to accept the connection. In
 * this case, a new socket is automatically allocated for the connection.
 *
 * To actively connect to a remote host, call tcp_connect() and wait for an
 * event signalling a successful connection attempt.
 *
 * Use tcp_write() and tcp_read() to send and receive data over a socket.
 * Events are generated whenever data was successfully sent or received. Use
 * tcp_buffer_used_rx() to check how much data is waiting to be read from a socket.
 *
 * @{
 */
/**
 * \file
 * TCP implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

#define TCP_FLAG_URG 0x20
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_FIN 0x01

#define TCP_OPT_EOL 0x00
#define TCP_OPT_NOP 0x01
#define TCP_OPT_MSS 0x02

#define TCP_OPT_MSS_LENGTH 4

enum tcp_state
{
	TCP_STATE_UNUSED = 0,
	TCP_STATE_CLOSED,
	TCP_STATE_LISTEN,
	TCP_STATE_NOT_ACCEPTED,
	TCP_STATE_ACCEPTED,
	TCP_STATE_SYN_SENT,
	TCP_STATE_SYN_RECEIVED,
	TCP_STATE_ESTABLISHED,
	TCP_STATE_START_CLOSE,
	TCP_STATE_FIN_WAIT_1,
	TCP_STATE_FIN_WAIT_2,
	TCP_STATE_CLOSING,
	TCP_STATE_TIME_WAIT,
	TCP_STATE_CLOSE_WAIT,
	TCP_STATE_LAST_ACK
};

struct tcp_tcb
{
	enum tcp_state state;
	tcp_callback callback;
	uint8_t ip[4];
	uint16_t port_source;
	uint16_t port_destination;
	uint32_t send_base;
	uint16_t send_next;
	uint32_t acked;
	uint16_t mss;
	uint16_t window;
	uint8_t timeout;
	uint8_t retx;
	uint8_t rto;
	uint8_t sa;
	uint8_t sv;
	struct tcp_queue* queue;
};

struct tcp_header
{
	uint16_t port_source;
	uint16_t port_destination;
	uint32_t seq;
	uint32_t ack;
	uint8_t offset;
	uint8_t flags;
	uint16_t window;
	uint16_t checksum;
	uint16_t urgent;
};

static struct tcp_tcb tcp_tcbs[TCP_MAX_SOCKET_COUNT];

#define FOREACH_TCB(tcb) \
	for((tcb) = &tcp_tcbs[0]; (tcb) < &tcp_tcbs[TCP_MAX_SOCKET_COUNT]; ++(tcb))

static void tcp_interval(int timer);

static void tcp_process_timeout(struct tcp_tcb* tcb);
static bool tcp_state_machine(struct tcp_tcb* tcb, const struct tcp_header* packet, uint16_t packet_len, const uint8_t* ip);

static bool tcp_check_sequence(const struct tcp_tcb* tcb, const struct tcp_header* packet, uint16_t packet_len);
static void tcp_parse_options(struct tcp_tcb* tcb, const struct tcp_header* packet);

static struct tcp_tcb* tcp_tcb_alloc();
static void tcp_tcb_free(struct tcp_tcb* tcb);
static void tcp_tcb_reset(struct tcp_tcb* tcb);
static int tcp_tcb_socket(const struct tcp_tcb* tcb);

static bool tcp_port_is_used(uint16_t port);
static uint16_t tcp_port_find_unused();
static uint16_t tcp_calc_checksum(const uint8_t* ip_destination, const struct tcp_header* packet, uint16_t packet_len);

static bool tcp_send_packet(struct tcp_tcb* tcb, uint8_t flags, bool send_data);
static bool tcp_send_rst(const uint8_t* ip_destination, const struct tcp_header* packet, uint16_t packet_len);

/**
 * Initializes the TCP layer.
 */
void tcp_init()
{
	memset(tcp_tcbs, 0, sizeof(tcp_tcbs));

	int timer = timer_alloc(tcp_interval, 0);
	timer_set(timer, 1000);
}

/**
 * Forwards incoming packets to the appropriate TCP socket.
 *
 * \note This function is used internally and should not be explicitly called by applications.
 *
 * \returns \c true if a matching socket was found, \c false if the packet was discarded.
 */
bool tcp_handle_packet(const uint8_t* ip, const struct tcp_header* packet, uint16_t packet_len)
{
	struct tcp_tcb* tcb;
	struct tcp_tcb* tcb_selected = 0;

	if(packet_len < sizeof(*packet))
		return false;

	/* test checksum */
	if(tcp_calc_checksum(ip, packet, packet_len) != ntoh16(packet->checksum))
		/* invalid checksum */
		return false;

	FOREACH_TCB(tcb) {
		if(tcb->state == TCP_STATE_UNUSED)
			continue;
		if(tcb->port_source != ntoh16(packet->port_destination))
			continue;
		if(tcb->state == TCP_STATE_LISTEN)
		{
			tcb_selected = tcb;
			continue;
		}
		if(tcb->port_destination != ntoh16(packet->port_source))
			continue;
		if(memcmp(ip, tcb->ip, sizeof(tcb->ip)) != 0)
			continue;

		tcb_selected = tcb;
		break;
	}

	if(tcb_selected) {
		return tcp_state_machine(tcb_selected, packet, packet_len, ip);
	}

	/* no tcb found, generate RST */
	tcp_send_rst(ip, packet, packet_len);

	return false;
}

/**
 * Global TCP socket timeout handler.
 *
 * \param[in] timer Timer identifier which generated the timeout event.
 * \param[in] user Pointer to user-defined data.
 */
void tcp_interval(int timer)
{
	struct tcp_tcb* tcb;

	FOREACH_TCB(tcb) {
		if(tcb->state != TCP_STATE_UNUSED && tcb->timeout > 0) {
			if( !(--tcb->timeout) ) {
				tcp_process_timeout(tcb);
			}
		}
	}

	timer_set(timer, 1000);
}

/**
 * Allocates a TCP socket.
 *
 * The given callback function will be called whenever an event is
 * generated for the socket.
 *
 * \param[in] callback A pointer to the event callback function.
 * \returns A non-negative socket identifier on success, \c -1 on failure.
 */
int tcp_socket_alloc(tcp_callback callback)
{
	if(!callback)
		return -1;

	/* search free tcb */
	struct tcp_tcb* tcb = tcp_tcb_alloc();
	if(!tcb)
		return -1;

	tcb->state = TCP_STATE_CLOSED;
	tcb->callback = callback;

	return tcp_tcb_socket(tcb);
}

/**
 * Deallocates a TCP socket.
 *
 * After freeing the socket it can no longer be used.
 *
 * \param[in] socket The identifier of the socket to deallocate.
 * \returns \c true if the socket has been deallocated, \c false on failure.
 */
bool tcp_socket_free(int socket)
{
	if(!tcp_socket_valid(socket))
		return false;

	/* abort connection regardless of its state */
	tcp_tcb_free(&tcp_tcbs[socket]);
	return true;
}

/**
 * Actively opens a connection to the given remote host and port.
 *
 * When the connection was successfully established, a TCP_EVT_CONN_ESTABLISHED
 * event is generated for the socket.
 *
 * \param[in] socket The identifier of the socket over which the connection will be established.
 * \param[in] ip The buffer containing the remote IP address.
 * \param[in] port The port on the remote host.
 * \returns \c true if the connection initiation was started successfully, \c false otherwise.
 */
bool tcp_connect(int socket, const uint8_t* ip, uint16_t port)
{
	if(!tcp_socket_valid(socket) || !ip || port < 1)
		return false;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_CLOSED)
		return false;
	
	/* clear connection state */
	tcp_tcb_reset(tcb);

	/* allocate data queue or clear it */
	tcb->queue = tcp_queue_alloc();
	if(!tcb->queue)
		return false;

	/* initialize connection state */
	memcpy(tcb->ip, ip, sizeof(tcb->ip));
	tcb->port_source = tcp_port_find_unused();
	tcb->port_destination = port;
	tcb->timeout = 10; /* start state machine on next timer event */

	return true;
}

/**
 * Actively closes a connection.
 *
 * When the connection has been closed, a TCP_EVT_CONN_CLOSED event is generated
 * for the socket.
 *
 * \param[in] socket The identifier of the socket whose connection should get closed.
 * \returns \c true if the connection shutdown was started successfully, \c false otherwise.
 */
bool tcp_disconnect(int socket)
{
	if(!tcp_socket_valid(socket))
		return false;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	switch(tcb->state)
	{
		case TCP_STATE_LISTEN:
			tcb->state = TCP_STATE_CLOSED;
			return true;
		case TCP_STATE_ESTABLISHED:
			tcb->state = TCP_STATE_START_CLOSE;
			if(tcb->timeout < 1)
				tcb->timeout = 1;
			return true;
		default:
			/* TODO: handle other cases */
			break;
	}

	return false;
}

/**
 * Starts listening on a local port for incoming connections.
 *
 * When an incoming connection is detected, a TCP_EVT_CONN_INCOMING event is
 * generated for the socket and tcp_accept() should be called to accept the connection.
 *
 * \param[in] socket The identifier of the port on which to listen for connections.
 * \param[in] port The number of the port on which to listen.
 * \returns \c true if the socket now listens for connections, \c false otherwise.
 */
bool tcp_listen(int socket, uint16_t port)
{
	if(!tcp_socket_valid(socket) || port < 1)
		return false;

	/* make sure listening port is not already active */
	if(tcp_port_is_used(port))
		return false;

	/* ensure socket is currently not connected */
	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_CLOSED)
		return false;
	
	/* initialize connection state */
	tcp_tcb_reset(tcb);
	tcb->state = TCP_STATE_LISTEN;
	tcb->port_source = port;

	return true;
}

/**
 * Accepts an incoming connection.
 *
 * This function has to be called in the event callback function
 * of a listening socket when a TCP_EVT_CONN_INCOMING event is
 * being handled. tcp_accept() returns a new socket which is
 * from now on used to handle the new connection. The given callback
 * function is attached to this new socket.
 *
 * \param[in] socket The identifier of the listening socket for which the event was generated.
 * \param[in] callback The callback function which should be attached to the new socket.
 * \returns The non-negative identifier of the new socket on success, \c -1 on failure.
 */
bool tcp_accept(int socket, tcp_callback callback)
{
	if(!tcp_socket_valid(socket))
		return false;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_NOT_ACCEPTED)
		return false;

	tcb->state = TCP_STATE_ACCEPTED;

	return true;
}

/**
 * Writes data for transmission to the socket.
 *
 * When (part of) the data has been successfully sent and acknowledged by the remote
 * host, a TCP_EVT_DATA_SENT event is generated for the socket.
 *
 * \param[in] socket The identifier of the socket over which the data will be sent.
 * \param[in] data A pointer to the buffer which contains the data to be sent.
 * \param[in] data_len The number of data bytes to send.
 * \returns The non-negative number of bytes queued for transmission on success, \c -1 on failure.
 */
int16_t tcp_write(int socket, const uint8_t* data, uint16_t data_len)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_ESTABLISHED)
		return -1;

	if(data_len > INT16_MAX)
		data_len = INT16_MAX;

	/* put data into queue */
	int16_t queued = tcp_queue_put_tx(tcb->queue, data, data_len);

	/* try to transmit on next timeout */
	if( tcp_queue_used_tx(tcb->queue) > 0 ) {
		tcb->timeout = 1;
	}

	return queued;
}

/**
 * Writes a string from flash memory space to the socket.
 *
 * When (part of) the data has been successfully sent and acknowledged by the remote
 * host, a TCP_EVT_DATA_SENT event is generated for the socket.
 *
 * \param[in] socket The identifier of the socket over which the data will be sent.
 * \param[in] str A pointer to the string in flash memory space.
 * \returns The non-negative number of bytes queued for transmission on success, \c -1 on failure.
 */
int16_t tcp_write_string_P(int socket, PGM_P str)
{
	int16_t queued = 0;
	int16_t len;
	char buffer[32];
	do
	{
		strncpy_P(buffer, str, sizeof(buffer));
		if(buffer[sizeof(buffer) - 1] != '\0')
			len = sizeof(buffer);
		else
			len = strlen(buffer);

		int16_t written = tcp_write(socket, (uint8_t*) buffer, len);
		if(written < 0)
			return -1;

		queued += written;
		str += written;

		if(written < len)
			return queued;

	} while(len == sizeof(buffer));

	return queued;
}

/**
 * Reads data which was received over a socket.
 *
 * This function is usually used in response to a TCP_EVT_DATA_RECEIVED event
 * which has been generated for the socket.
 *
 * \param[in] socket The identifier of the socket from which the data should be read.
 * \param[out] buffer A pointer to the buffer to which the received data should be written.
 * \param[in] buffer_len The maximum number of bytes to read from the socket.
 * \returns The non-negative number of bytes actually written to the buffer on success, \c -1 on failure.
 */
int16_t tcp_read(int socket, uint8_t* buffer, uint16_t buffer_len)
{
	int16_t peeked = tcp_peek(socket, buffer, 0, buffer_len);
	if(peeked < 0)
		return peeked;

	return tcp_skip(socket, peeked);
}

/**
 * Reads data which was received over a socket, without actually removing it from the input buffer.
 *
 * \param[in] socket The identifier of the socket from which the data should be read.
 * \param[out] buffer A pointer to the buffer to which the received data should be written.
 * \param[in] offset Skip the first \a offset bytes before starting to read.
 * \param[in] buffer_len The maximum number of bytes to read.
 * \returns The non-negative number of bytes actually written to the buffer on success, \c -1 on failure.
 */
int16_t tcp_peek(int socket, uint8_t* buffer, uint16_t offset, uint16_t buffer_len)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state == TCP_STATE_UNUSED)
		return -1;

	if(buffer_len > INT16_MAX)
		buffer_len = INT16_MAX;

	/* peek data from queue */
	return tcp_queue_peek_rx(tcb->queue, buffer, offset, buffer_len);
}

/**
 * Removes data from the receive buffer of a socket without reading it.
 *
 * \param[in] socket The identifier of the socket from which the data should be removed.
 * \param[in] len The maximum number of bytes to remove.
 * \returns The number of bytes actually removed.
 */
int16_t tcp_skip(int socket, uint16_t len)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state == TCP_STATE_UNUSED)
		return -1;

	if(len > INT16_MAX)
		len = INT16_MAX;

	/* peek data from queue */
	return tcp_queue_skip_rx(tcb->queue, len);
}

/**
 * Declares unused transmit buffer space of a socket as valid.
 *
 * This is only useful if tcp_buffer_tx() has been used before 
 * to retrieve and fill the unused buffer space with valid data.
 *
 * \param[in] socket The identifier of the socket over which the data will be sent.
 * \param[in] len The number of data bytes to declare as valid.
 * \returns The number of bytes which have been declared as valid on success, \c -1 on failure.
 */
int16_t tcp_reserve(int socket, uint16_t len)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_ESTABLISHED)
		return -1;

	if(len > INT16_MAX)
		len = INT16_MAX;

	/* get pointer to the empty part of the queue transmit buffer */
	int16_t queued = tcp_queue_reserve_tx(tcb->queue, len);

	/* try to transmit on next timeout */
	if(tcp_queue_used_tx(tcb->queue) > 0)
		tcb->timeout = 1;

	return queued;
}

/**
 * Retrieves a pointer to the occupied part of the receive buffer space of a socket.
 *
 * The buffer is guaranteed to be linear and contiguous. Its size is
 * retrieved with tcp_buffer_used_rx().
 *
 * \note This function can be quite expensive, especially with a full receive buffer.
 *
 * \param[in] socket The identifier of the socket of which to retrieve the occupied receive buffer space.
 * \returns A pointer to the occupied buffer space on success, \c NULL on failure.
 */
uint8_t* tcp_buffer_rx(int socket)
{
	if(!tcp_socket_valid(socket))
		return 0;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state == TCP_STATE_UNUSED)
		return 0;

	/* get pointer to the occupied part of the queue receive buffer */
	return tcp_queue_used_buffer_rx(tcb->queue);
}

/**
 * Retrieves a pointer to the empty part of the transmit buffer space of a socket.
 *
 * The buffer is guaranteed to be linear and contiguous. Its size is
 * retrieved with tcp_buffer_space_tx(). After writing to the buffer, be sure to
 * call tcp_reserve() to actually add the data to the transmit buffer.
 *
 * \attention Make sure the transmit buffer is not modified while the
 * empty buffer space is being used.
 *
 * \param[in] socket The identifier of the socket of which to retrieve the empty transmit buffer space.
 * \returns A pointer to the empty transmit buffer space on success, \c NULL on failure.
 */
uint8_t* tcp_buffer_tx(int socket)
{
	if(!tcp_socket_valid(socket))
		return 0;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_ESTABLISHED)
		return 0;

	/* get pointer to the empty part of the queue transmit buffer */
	return tcp_queue_space_buffer_tx(tcb->queue);
}

/**
 * Retrieves the number of available data bytes waiting to be read from the socket.
 *
 * \param[in] socket The identifier of the socket to ask for available data.
 * \returns The non-negative number of available data bytes on success, \c -1 on failure.
 */
int16_t tcp_buffer_used_rx(int socket)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state == TCP_STATE_UNUSED)
		return -1;

	return tcp_queue_used_rx(tcb->queue);
}

/**
 * Retrieves the number of data bytes which currently can be received until the receive
 * buffer of the socket is full.
 *
 * \param[in] socket The identifier of the socket to ask for unused receive buffer space.
 * \returns The non-negative number of unused receive buffer bytes on success, \c -1 on failure.
 */
int16_t tcp_buffer_space_rx(int socket)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state == TCP_STATE_UNUSED)
		return -1;

	return tcp_queue_space_rx(tcb->queue);
}

/**
 * Retrieves the number of data bytes which have been written to the socket but which have not been
 * sent to the remote host or which have not been acknowledged yet.
 *
 * \param[in] socket The identifier of the socket of which to get the occupied transmit buffer space.
 * \returns The non-negative number of already used transmit buffer bytes on success, \c -1 on failure.
 */
int16_t tcp_buffer_used_tx(int socket)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_ESTABLISHED)
		return -1;

	return tcp_queue_used_tx(tcb->queue);
}

/**
 * Retrieves the number of data bytes which currently can be written to the socket.
 *
 * \param[in] socket The identifier of the socket of which to get the available transmit buffer space.
 * \returns The non-negative number of unused transmit buffer bytes on success, \c -1 on failure.
 */
int16_t tcp_buffer_space_tx(int socket)
{
	if(!tcp_socket_valid(socket))
		return -1;

	struct tcp_tcb* tcb = &tcp_tcbs[socket];
	if(tcb->state != TCP_STATE_ESTABLISHED)
		return -1;

	return tcp_queue_space_tx(tcb->queue);
}

/**
 * The part of the TCP state machine which handles timeouts.
 *
 * \param[in] tcb The transfer control block for which to handle a timeout.
 */
void tcp_process_timeout(struct tcp_tcb* tcb)
{
	if(!tcb)
		return;

	int socket = tcp_tcb_socket(tcb);
	if(socket < 0)
		return;

#if TCP_DEBUG
	printf_P(PSTR("sm: sock: %d state: %d timeout\n"),
			 socket,
			 tcb->state
			);
#endif

	switch(tcb->state)
	{
		case TCP_STATE_CLOSED:
		{
			/* send SYN packet */
			if(!tcp_send_packet(tcb, TCP_FLAG_SYN, false))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_ERROR);
				return;
			}

			tcb->state = TCP_STATE_SYN_SENT;
			tcb->timeout = TCP_TIMEOUT_GENERIC;
			tcb->retx = 0;

			break;
		}
		case TCP_STATE_SYN_SENT:
		{
			if(++tcb->retx > TCP_MAX_RETRY)
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_TIMEOUT);
				return;
			}

			/* resend SYN packet */
			if(!tcp_send_packet(tcb, TCP_FLAG_SYN, false))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_ERROR);
				return;
			}

			tcb->timeout = TCP_TIMEOUT_GENERIC;

			break;
		}
		case TCP_STATE_SYN_RECEIVED:
		{
			if(++tcb->retx > TCP_MAX_RETRY)
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_TIMEOUT);
				return;
			}

			/* send SYNACK packet */
			if(!tcp_send_packet(tcb, TCP_FLAG_SYN | TCP_FLAG_ACK, false))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_ERROR);
				return;
			}

			tcb->timeout = TCP_TIMEOUT_GENERIC;

			break;
		}
		case TCP_STATE_ESTABLISHED:
		case TCP_STATE_START_CLOSE:
		{
			if(tcp_queue_used_tx(tcb->queue) > 0)
			{
				if(++tcb->retx > TCP_MAX_RETRY)
				{
					tcb->state = TCP_STATE_CLOSED;
					tcb->callback(socket, TCP_EVT_TIMEOUT);

					return;
				}

				/* resend queued data */
				tcb->send_next = 0;
				tcp_send_packet(tcb, TCP_FLAG_ACK, true);
			}
			else if(tcb->state == TCP_STATE_START_CLOSE)
			{
				/* send FIN to actively initiate closing the connection */
				tcp_send_packet(tcb, TCP_FLAG_ACK | TCP_FLAG_FIN, true);

				tcb->state = TCP_STATE_FIN_WAIT_1;
				tcb->timeout = TCP_TIMEOUT_GENERIC;
				tcb->retx = 0;
			}
			else
			{
				tcb->callback(socket, TCP_EVT_CONN_IDLE);
				tcb->timeout = 1;
			}

			break;
		}
		case TCP_STATE_CLOSE_WAIT:
		{
			/* send FIN */
			tcb->send_next = 0;
			if(!tcp_send_packet(tcb, TCP_FLAG_FIN | TCP_FLAG_ACK, true))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_ERROR);
				return;
			}

			tcb->state = TCP_STATE_LAST_ACK;
			tcb->timeout = TCP_TIMEOUT_GENERIC;
			tcb->retx = 0;

			break;
		}
		case TCP_STATE_FIN_WAIT_1:
		case TCP_STATE_CLOSING:
		case TCP_STATE_LAST_ACK:
		{
			if(++tcb->retx > TCP_MAX_RETRY)
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_TIMEOUT);
				return;
			}

			/* resend FIN */
			tcb->send_next = 0;
			if(!tcp_send_packet(tcb, TCP_FLAG_FIN | TCP_FLAG_ACK, true))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->callback(socket, TCP_EVT_ERROR);
				return;
			}
			tcb->timeout = TCP_TIMEOUT_GENERIC;

			break;
		}
		case TCP_STATE_FIN_WAIT_2:
		{
			tcb->state = TCP_STATE_CLOSED;
			tcb->callback(socket, TCP_EVT_TIMEOUT);
			break;
		}
		case TCP_STATE_TIME_WAIT:
		{
			/* finally close tcb */
			tcb->state = TCP_STATE_CLOSED;
			tcb->callback(socket, TCP_EVT_CONN_CLOSED);

			break;
		}
		case TCP_STATE_LISTEN:
		case TCP_STATE_NOT_ACCEPTED:
		case TCP_STATE_ACCEPTED:
		case TCP_STATE_UNUSED:
		default:
			break;
	}
}

/**
 * The part of the TCP state machine which handles incoming packets.
 *
 * \param[in] tcb The transfer control block for which to handle a packet.
 * \param[in] packet A pointer to the receive buffer containing the packet.
 * \param[in] packet_len The length of the packet in bytes.
 * \param[in] ip A pointer to the buffer containing the IP address of the host which sent the packet.
 * \returns \c true if the packet was successfully handled, \c false otherwise.
 */
bool tcp_state_machine(struct tcp_tcb* tcb, const struct tcp_header* packet, uint16_t packet_len, const uint8_t* ip)
{
    /* This function implements the TCP connection state machine to properly
     * support establishing and closing connections. Basically it is a
     * realisation of the following state diagram, as contained in RFC 793
     * and corrected in RFC 1122:
     *
     *                              +---------+ ---------\      active OPEN  
     *                              |  CLOSED |            \    -----------  
     *                              +---------+<---------\   \   create TCB  
     *                                |     ^              \   \  snd SYN    
     *                   passive OPEN |     |   CLOSE        \   \           
     *                   ------------ |     | ----------       \   \         
     *   rcv RST          create TCB  |     | delete TCB         \   \       
     *   -------                      V     |                      \   \     
     *      x                       +---------+            CLOSE    |    \   
     *         -------------------->|  LISTEN |          ---------- |     |  
     *       /                      +---------+          delete TCB |     |  
     *      |            rcv SYN      |     |     SEND              |     |  
     *      |           -----------   |     |    -------            |     V  
     * +---------+      snd SYN,ACK  /       \   snd SYN          +---------+
     * |         |<-----------------           ------------------>|         |
     * |   SYN   |                    rcv SYN                     |   SYN   |
     * |   RCVD  |<-----------------------------------------------|   SENT  |
     * |         |                  snd SYN,ACK                   |         |
     * |         |------------------           -------------------|         |
     * +---------+   rcv ACK of SYN  \       /  rcv SYN,ACK       +---------+
     *   |           --------------   |     |   -----------                  
     *   |                  x         |     |     snd ACK                    
     *   |                            V     V                                
     *   |  CLOSE                   +---------+                              
     *   | -------                  |  ESTAB  |                              
     *   | snd FIN                  +---------+                              
     *   |                   CLOSE    |     |    rcv FIN                     
     *   V                  -------   |     |    -------                     
     * +---------+          snd FIN  /       \   snd ACK          +---------+
     * |  FIN    |<-----------------           ------------------>|  CLOSE  |
     * | WAIT-1  |------------------                              |   WAIT  |
     * +---------+          rcv FIN  \                            +---------+
     *   | rcv ACK of FIN   -------   |                            CLOSE  |  
     *   | --------------   snd ACK   |                           ------- |  
     *   V        x                   V                           snd FIN V  
     * +---------+                  +---------+                   +---------+
     * |FINWAIT-2|                  | CLOSING |                   | LAST-ACK|
     * +---------+                  +---------+                   +---------+
     *   |                rcv ACK of FIN |                 rcv ACK of FIN |  
     *   |  rcv FIN       -------------- |    Timeout=2MSL -------------- |  
     *   |  -------              x       V    ------------        x       V  
     *    \ snd ACK                 +---------+delete TCB         +---------+
     *     ------------------------>|TIME WAIT|------------------>| CLOSED  |
     *                              +---------+                   +---------+
     */

	if(!tcb || !packet || packet_len < sizeof(*packet) || !ip)
		return false;

	int socket = tcp_tcb_socket(tcb);
	if(socket < 0)
		return false;

#if TCP_DEBUG
	printf_P(PSTR("sm: sock: %d state: %d len: %d flags: %02x\n"),
			 socket,
			 tcb->state,
			 packet_len,
			 packet->flags
			);
#endif

	switch(tcb->state)
	{
		case TCP_STATE_CLOSED:
		{
			if(packet->flags & TCP_FLAG_RST)
				return false;

			/* send RST */
			tcp_send_rst(ip, packet, packet_len);

			return true;
		}
		case TCP_STATE_LISTEN:
		{
			if(packet->flags & TCP_FLAG_RST)
				return false;

			if(packet->flags & TCP_FLAG_ACK)
			{
				/* send RST */
				tcp_send_rst(ip, packet, packet_len);
				return false;
			}

			if(!(packet->flags & TCP_FLAG_SYN))
			{
				/* send RST */
				tcp_send_rst(ip, packet, packet_len);
				return false;
			}

			struct tcp_tcb* tcb_new = tcp_tcb_alloc();
			struct tcp_queue* tcb_queue = tcp_queue_alloc();
			if(!tcb_new || !tcb_queue)
			{
				/* We do not have any unused connection slots, so
				 * we can not handle the connection at the moment.
				 * Just hope that the remote host will resend the
				 * SYN later when we might have an unused connection.
				 */

				tcp_tcb_free(tcb_new);
				tcp_queue_free(tcb_queue);
				return false;
			}

			/* create a new tcb which the application has to accept */
			tcp_tcb_reset(tcb_new);
			memcpy(tcb_new->ip, ip, sizeof(tcb_new->ip));
			tcb_new->state = TCP_STATE_NOT_ACCEPTED;
			tcb_new->callback = tcb->callback;
			tcb_new->port_source = tcb->port_source;
			tcb_new->port_destination = ntoh16(packet->port_source);
			tcb_new->acked = ntoh32(packet->seq) + 1;
			tcb_new->queue = tcb_queue;

			/* ask application wether it accepts the new tcb */
			tcb_new->callback(tcp_tcb_socket(tcb_new), TCP_EVT_CONN_INCOMING);

			/* abort if application does not accept tcb */
			if(tcb_new->state != TCP_STATE_ACCEPTED)
			{
				/* send RST */
				tcp_send_rst(ip, packet, packet_len);
				tcp_tcb_free(tcb_new);
				return true;
			}

			/* parse options to get max segment size */
			tcp_parse_options(tcb_new, packet);

			/* send SYNACK packet */
			if(!tcp_send_packet(tcb_new, TCP_FLAG_SYN | TCP_FLAG_ACK, false))
			{
				tcb_new->state = TCP_STATE_CLOSED;
				tcb_new->callback(tcp_tcb_socket(tcb_new), TCP_EVT_ERROR);
				return false;
			}

			tcb_new->state = TCP_STATE_SYN_RECEIVED;
			tcb_new->timeout = TCP_TIMEOUT_GENERIC;
			tcb_new->retx = 0;

			return true;
		}
		case TCP_STATE_SYN_SENT:
		{
			if(packet->flags & TCP_FLAG_ACK)
			{
				if(ntoh32(packet->ack) != tcb->send_base + 1)
				{
					if(!(packet->flags & TCP_FLAG_RST))
						/* send RST */
						tcp_send_rst(ip, packet, packet_len);
					return false;
				}

				if(packet->flags & TCP_FLAG_RST)
				{
					tcb->state = TCP_STATE_CLOSED;
					tcb->timeout = 0;
					tcb->callback(socket, TCP_EVT_RESET);
					return true;
				}

				++tcb->send_base; /* the acked SYN counts as an octet */
			}

			if(packet->flags & TCP_FLAG_RST)
				return false;

			if(packet->flags & TCP_FLAG_SYN)
			{
				/* parse options to get max segment size */
				tcp_parse_options(tcb, packet);

				tcb->acked = ntoh32(packet->seq) + 1;

				if(packet->flags & TCP_FLAG_ACK)
				{
					/* ack SYN packet */
					if(!tcp_send_packet(tcb, TCP_FLAG_ACK, false))
					{
						tcb->state = TCP_STATE_CLOSED;
						tcb->timeout = 0;
						tcb->callback(socket, TCP_EVT_ERROR);
						return false;
					}

					tcb->state = TCP_STATE_ESTABLISHED;
					tcb->timeout = TCP_TIMEOUT_IDLE; /* detect when connection goes idle */
					tcb->retx = 0;

					tcb->callback(socket, TCP_EVT_CONN_ESTABLISHED);

					return true;
				}

				/* send SYNACK packet */
				if(!tcp_send_packet(tcb, TCP_FLAG_SYN | TCP_FLAG_ACK, false))
				{
					tcb->state = TCP_STATE_CLOSED;
					tcb->timeout = 0;
					tcb->callback(socket, TCP_EVT_ERROR);
					return false;
				}

				tcb->state = TCP_STATE_SYN_RECEIVED;
				tcb->timeout = TCP_TIMEOUT_GENERIC;
				tcb->retx = 0;

				return true;
			}

			return false;
		}
		default:
			break;
	}

	/* check for sequence number plausibility */
	if(!tcp_check_sequence(tcb, packet, packet_len))
	{
		/* Packet is out of sequence.
		 * Tell our position to the remote host.
		 */
		tcp_send_packet(tcb, TCP_FLAG_ACK, false);

		return false;
	}

	/* abort connection on RST or irregular SYN packets */
	switch(tcb->state)
	{
		case TCP_STATE_SYN_RECEIVED:
		case TCP_STATE_ESTABLISHED:
		case TCP_STATE_START_CLOSE:
		case TCP_STATE_FIN_WAIT_1:
		case TCP_STATE_FIN_WAIT_2:
		case TCP_STATE_CLOSE_WAIT:
		case TCP_STATE_CLOSING:
		case TCP_STATE_LAST_ACK:
		case TCP_STATE_TIME_WAIT:
		{
			if(packet->flags & (TCP_FLAG_RST | TCP_FLAG_SYN))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->timeout = 0;
				tcb->callback(socket, TCP_EVT_RESET);
				return false;
			}

			break;
		}
		default:
			break;
	}

	/* all the following requires ACK */
	if(!(packet->flags & TCP_FLAG_ACK))
		return false;

	switch(tcb->state)
	{
		case TCP_STATE_SYN_RECEIVED:
		{
			if(ntoh32(packet->ack) != tcb->send_base + 1)
				return false;

			++tcb->send_base; /* SYN counts as an octet */

			tcb->state = TCP_STATE_ESTABLISHED;
			tcb->timeout = TCP_TIMEOUT_IDLE; /* timeout for idle event generation */

			tcb->callback(socket, TCP_EVT_CONN_ESTABLISHED);

			break;
		}
		case TCP_STATE_ESTABLISHED:
		case TCP_STATE_START_CLOSE:
		case TCP_STATE_FIN_WAIT_1:
		case TCP_STATE_FIN_WAIT_2:
		case TCP_STATE_CLOSE_WAIT:
		case TCP_STATE_CLOSING:
		{
			uint32_t packet_ack = ntoh32(packet->ack);
			uint16_t queue_used = tcp_queue_used_tx(tcb->queue);

			if(tcb->send_base == packet_ack)
				break;

			if(((tcb->send_base + queue_used > tcb->send_base) && packet_ack > tcb->send_base && packet_ack <= tcb->send_base + queue_used) ||
			   ((tcb->send_base + queue_used < tcb->send_base) && (packet_ack > tcb->send_base || packet_ack <= tcb->send_base + queue_used))
			  )
			{
				/* ACK lies within window */
				uint16_t bytes_acked = packet_ack - tcb->send_base;

				/* remove acknowledged bytes */
				tcp_queue_skip_tx(tcb->queue, bytes_acked);
				/* advance sequence number */
				tcb->send_base += bytes_acked;
				if(tcb->send_next >= bytes_acked)
					tcb->send_next -= bytes_acked;
				else
					tcb->send_next = 0;

				/* get current window of remote host */
				tcb->window = ntoh16(packet->window);

				/* do round trip time estimation */
				if(tcb->timeout > 0 && tcb->retx == 0)
				{
					/* Taken out of
					 * "Congestion Avoidance and Control"
					 * by Van Jacobson in
					 * "Proceedings of SIGCOMM '88"
					 * Stanford, 1988, ACM.
					 */
					int8_t m = tcb->rto - tcb->timeout;
					m -= tcb->sa >> 3;
					tcb->sa += m;
					if(m < 0)
						m = -m;
					m -= tcb->sv >> 2;
					tcb->sv += m;
					tcb->rto = (tcb->sa >> 3) + tcb->sv;
				}

				/* reset retransmission counter */
				tcb->retx = 0;

				/* inform application that some data has been acknowledged */
				tcb->callback(socket, TCP_EVT_DATA_SENT);

				if(tcp_queue_used_tx(tcb->queue) > 0)
				{
					/* use this opportunity to keep data flowing */
					if(!tcp_send_packet(tcb, TCP_FLAG_ACK, true))
					{
						tcb->state = TCP_STATE_CLOSED;
						tcb->timeout = 0;
						tcb->callback(socket, TCP_EVT_ERROR);
						return false;
					}
				}
				else if(tcb->state == TCP_STATE_START_CLOSE)
				{
					/* The user may call tcp_disconnect() even if the socket still has
					 * data in its outgoing queue. But now, all outgoing data has been
					 * acknowledged.
					 * As the FIN is sent from a timeout event, we shorten the timeout
					 * period, which may be much longer depending on the round trip
					 * time estimation for the last data packet.
					 */
					tcb->timeout = 1;
				}
				else
				{
					/* start idle timeout */
					tcb->timeout = TCP_TIMEOUT_IDLE;
				}

				break;
			}
			else if(packet_ack < tcb->send_base)
			{
				/* duplicate ACK, ignore */
				break;
			}
			else if(packet_ack == tcb->send_base + 1)
			{
				if(tcb->state == TCP_STATE_FIN_WAIT_1)
				{
					/* our FIN has been acknowledged */
					tcb->state = TCP_STATE_FIN_WAIT_2;
					tcb->timeout = TCP_TIMEOUT_GENERIC;

					++tcb->send_base;

					break;
				}
				else if(tcb->state == TCP_STATE_FIN_WAIT_2)
				{
					/* nothing, wait for FIN */
					break;
				}
				else if(tcb->state == TCP_STATE_CLOSING)
				{
					/* our FIN has been acknowledged */
					tcb->state = TCP_STATE_TIME_WAIT;
					tcb->timeout = TCP_TIMEOUT_TIME_WAIT;

					return true;
				}
			}

			/* ACK lies beyond window, nonsense */
			if(!tcp_send_packet(tcb, TCP_FLAG_ACK, true))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->timeout = 0;
				tcb->callback(socket, TCP_EVT_ERROR);
			}

			return false;
		}
		case TCP_STATE_LAST_ACK:
		{
			if(tcp_queue_used_tx(tcb->queue) == 0 &&
			   ntoh32(packet->ack) == tcb->send_base + 1
			  )
			{
				/* remote host has accepted to close connection */
				tcb->state = TCP_STATE_CLOSED;
				tcb->timeout = 0;
				tcb->callback(socket, TCP_EVT_CONN_CLOSED);
				return true;
			}

			return false;
		}
		case TCP_STATE_TIME_WAIT:
		{
			/* remote host has retransmitted his FIN, so just ACK it */

			if(!tcp_send_packet(tcb, TCP_FLAG_ACK, false))
			{
				tcb->state = TCP_STATE_CLOSED;
				tcb->timeout = 0;
				tcb->callback(socket, TCP_EVT_ERROR);
				return false;
			}

			tcb->timeout = TCP_TIMEOUT_GENERIC;
			break;
		}
		default:
			break;
	}

	/* process packet data */
	switch(tcb->state)
	{
		case TCP_STATE_ESTABLISHED:
		case TCP_STATE_START_CLOSE:
		case TCP_STATE_FIN_WAIT_1:
		case TCP_STATE_FIN_WAIT_2:
		{
			uint16_t packet_data_len = packet_len - (packet->offset >> 4) * 4;
			uint16_t data_new = 0;

			if(packet_data_len > 0)
			{
				/* put data into receive buffer */
				data_new = tcp_queue_put_rx(tcb->queue,
											(uint8_t*) (packet) + (packet->offset >> 4) * 4,
											packet_data_len
										   );

				tcb->acked += data_new;

				if(data_new > 0)
				{
					/* send ACK */
					if(!tcp_send_packet(tcb, TCP_FLAG_ACK, false))
					{
						tcb->state = TCP_STATE_CLOSED;
						tcb->timeout = 0;
						tcb->callback(socket, TCP_EVT_ERROR);
						return false;
					}

					/* start idle timeout */
					if(tcb->state == TCP_STATE_ESTABLISHED && tcp_queue_used_tx(tcb->queue) < 1)
						tcb->timeout = TCP_TIMEOUT_IDLE;

					/* inform application */
					tcb->callback(socket, TCP_EVT_DATA_RECEIVED);
				}
			}

			/* Prevent a FIN from being accepted if not all of
			 * the packet's data could be queued.
			 */
			if(data_new < packet_data_len)
				return true;

			break;
		}
		case TCP_STATE_CLOSE_WAIT:
		case TCP_STATE_CLOSING:
		case TCP_STATE_LAST_ACK:
		case TCP_STATE_TIME_WAIT:
		{
			/* FIN has been received before, ignore data */
			break;
		}
		default:
			break;
	}

	/* process FIN */
	if(packet->flags & TCP_FLAG_FIN)
	{
		if(tcb->state == TCP_STATE_CLOSED ||
		   tcb->state == TCP_STATE_LISTEN ||
		   tcb->state == TCP_STATE_SYN_SENT
		  )
			return false;

		++tcb->acked;

		/* send ACK */
		if(!tcp_send_packet(tcb, TCP_FLAG_ACK, true))
		{
			tcb->state = TCP_STATE_CLOSED;
			tcb->timeout = 0;
			tcb->callback(tcp_tcb_socket(tcb), TCP_EVT_ERROR);
			return false;
		}

		switch(tcb->state)
		{
			case TCP_STATE_SYN_RECEIVED:
			case TCP_STATE_ESTABLISHED:
			case TCP_STATE_START_CLOSE:
				/* TODO: maybe immediately send FIN here and enter LAST_ACK */
				tcb->state = TCP_STATE_CLOSE_WAIT;
				tcb->timeout = 1;
				break;
			case TCP_STATE_FIN_WAIT_1:
				/* TODO: validate: RFC793, p. 75 */
				tcb->state = TCP_STATE_CLOSING;
				tcb->timeout = TCP_TIMEOUT_GENERIC;
				tcb->retx = 0;
				break;
			case TCP_STATE_FIN_WAIT_2:
				tcb->state = TCP_STATE_TIME_WAIT;
			case TCP_STATE_TIME_WAIT:
				tcb->timeout = TCP_TIMEOUT_TIME_WAIT;
				break;
			default:
				break;;
		}
	}

	return true;
}

/**
 * Checks a TCP packet sequence number for validity.
 *
 * \param[in] tcb The transfer control block to which the packet belongs.
 * \param[in] packet The TCP packet whose sequence number should be checked.
 * \param[in] packet_len The length of the packet.
 * \returns \c true if the sequence number is valid, \c false otherwise.
 */
bool tcp_check_sequence(const struct tcp_tcb* tcb, const struct tcp_header* packet, uint16_t packet_len)
{
	if(!tcb || !packet)
		return false;

	/* Check for sequence number plausibility.
	 *
	 * We only accept packets with data which starts at the queue's
	 * left border, although RFC 793 wants that we accept everything
	 * (partially) fitting into the window.
	 */

	return tcb->acked == ntoh32(packet->seq);
}

/**
 * Parse the options header field of a TCP packet header.
 *
 * \param[in] tcb The transfer control block to which the packet belongs.
 * \param[in] packet The TCP packet whose options will be parsed.
 */
void tcp_parse_options(struct tcp_tcb* tcb, const struct tcp_header* packet)
{
	if(!tcb || !packet)
		return;

	const uint8_t* options_data = (const uint8_t*) (packet + 1);
	const uint8_t* options_end = (const uint8_t*) packet + (packet->offset >> 4) * 4;
	for(; options_data < options_end; ++options_data)
	{
		if(*options_data == TCP_OPT_EOL)
		{
			/* end of options */
			break;
		}
		else if(*options_data == TCP_OPT_NOP)
		{
			continue;
		}
		else if(*options_data == TCP_OPT_MSS && *(options_data + 1) == TCP_OPT_MSS_LENGTH)
		{
			/* max segment length */

			options_data += 2;
			tcb->mss = ntoh16(*((uint16_t*) options_data));
		}
		else if(*(options_data + 1) < 2)
		{
			/* invalid length, abort */
			break;
		}
		else
		{
			/* unknown option, skip */
			options_data += *(options_data + 1);
		}
	}
}

/**
 * Allocates a transfer control block.
 *
 * \returns A pointer to the allocated control block on success, NULL on failure.
 */
struct tcp_tcb* tcp_tcb_alloc()
{
	/* search free tcb */
	struct tcp_tcb* tcb;
	FOREACH_TCB(tcb)
	{
		if(tcb->state == TCP_STATE_UNUSED)
		{
			/* initialize tcb */
			memset(tcb, 0, sizeof(*tcb));

			tcb->state = TCP_STATE_CLOSED;

			return tcb;
		}
	}

	return 0;
}

/**
 * Deallocates a transfer control block.
 *
 * \param[in] tcb A pointer to the transfer control block to deallocate.
 */
void tcp_tcb_free(struct tcp_tcb* tcb)
{
	if(!tcb)
		return;

	tcp_queue_free(tcb->queue);

	tcb->state = TCP_STATE_UNUSED;
	tcb->queue = 0;
}

/**
 * Initializes or resets a transfer control block.
 *
 * \param[in] tcb A pointer to the transfer control block which will be reseted.
 */
void tcp_tcb_reset(struct tcp_tcb* tcb)
{
	if(!tcb)
		return;

	tcp_queue_free(tcb->queue);

	tcp_callback callback = tcb->callback;

	memset(tcb, 0, sizeof(*tcb));
	tcb->state = TCP_STATE_CLOSED;
	tcb->callback = callback;
	tcb->mss = 536;
	tcb->rto = TCP_TIMEOUT_RTT;
	tcb->sv = TCP_TIMEOUT_RTT / 2;
}

/**
 * Retrieve the socket identifier describing the given transfer control block.
 *
 * \param[in] tcb A pointer to the transfer control block.
 * \returns The socket identifier pointing to the transfer control block.
 */
int tcp_tcb_socket(const struct tcp_tcb* tcb)
{
	if(!tcb)
		return -1;

	int socket = ((int) tcb - (int) &tcp_tcbs[0]) / sizeof(*tcb);
	if(!tcp_socket_valid(socket))
		return -1;

	return socket;
}

/**
 * Determines wether the given port number is already used locally by any socket.
 *
 * \param[in] port The port number to check for usage.
 * \returns \c true if the given port is already in use, \c false otherwise.
 */
bool tcp_port_is_used(uint16_t port)
{
	struct tcp_tcb* tcb;
	
	FOREACH_TCB(tcb) {
		if( tcb->state != TCP_STATE_UNUSED &&
			tcb->state != TCP_STATE_CLOSED &&
			tcb->port_source == port
		) {
			return true;
		}
	}
	
	return false;
}

/**
 * Searches for a locally unused port number.
 *
 * \returns An unused port number.
 */
uint16_t tcp_port_find_unused()
{
	static uint16_t port_counter = 1023;
	
	do
	{
		if(++port_counter < 1024)
			port_counter = 1024;

	} while(tcp_port_is_used(port_counter));

	return port_counter;
}

/**
 * Calculates the checksum of a TCP packet.
 *
 * \param[in] ip_destination The remote IP address to which the packet gets sent or from which it was received.
 * \param[in] packet A pointer to the buffer containing the packet.
 * \param[in] packet_len The length in bytes of the packet.
 * \returns The 16-bit checksum of the packet.
 */
uint16_t tcp_calc_checksum(const uint8_t* ip_destination, const struct tcp_header* packet, uint16_t packet_len)
{
	/* pseudo header */
	uint16_t checksum = IP_PROTOCOL_TCP + packet_len;
	checksum = net_calc_checksum(checksum, ip_get_address(), 4, 4);
	checksum = net_calc_checksum(checksum, ip_destination, 4, 4);

	/* real package */
	return ~net_calc_checksum(checksum, (uint8_t*) packet, packet_len, 16);
}

/**
 * Send data queued for a connection.
 *
 * \param[in] tcb The transfer control block associated with the connection.
 * \param[in] flags The TCP flags which should be set in the packet(s).
 * \param[in] send_data Determines wether non-empty or empty packets should be sent.
 * \returns \c true if the packet(s) was/were successfully sent, \c false otherwise.
 */
bool tcp_send_packet(struct tcp_tcb* tcb, uint8_t flags, bool send_data)
{
	if(!tcb || (flags & TCP_FLAG_RST)) {
		return false;
	}

	/* For simplicity prevent sending data when
	 * transmitting SYNs.
	 */
	if(flags & TCP_FLAG_SYN) {
		send_data = false;
	}

	/* prepare packet header */
	struct tcp_header* packet_header = (struct tcp_header*) ip_get_buffer();
	memset(packet_header, 0, sizeof(*packet_header));
	packet_header->port_source = hton16(tcb->port_source);
	packet_header->port_destination = hton16(tcb->port_destination);
	packet_header->ack = hton32(tcb->acked);
	packet_header->window = hton16(tcp_queue_space_rx(tcb->queue));
	packet_header->urgent = HTON16(0x0000);

	uint16_t data_available = tcp_queue_used_tx(tcb->queue);
	uint8_t* packet_data = (uint8_t*) (packet_header + 1);
	uint16_t packet_header_size = sizeof(*packet_header);
	uint16_t packet_data_max = ip_get_buffer_size() - packet_header_size;
	uint32_t packet_seq = tcb->send_base;

	if(flags & TCP_FLAG_SYN) {
		/* send option informing about maximum segment size */
		if(packet_data_max >= sizeof(uint32_t)) {
			*((uint32_t*) packet_data) = HTON32(0x0204UL << 16 | TCP_MSS);
			packet_data += sizeof(uint32_t);
			packet_data_max -= sizeof(uint32_t);
			packet_header_size += sizeof(uint32_t);
		}
	}

	packet_header->offset = (packet_header_size / 4) << 4;

	/* take window and MSS of remote host into account */
	if(packet_data_max > tcb->mss) {
		packet_data_max = tcb->mss;
	}
	
	if(tcb->window > 0) {
		if(packet_data_max > tcb->window) {
			packet_data_max = tcb->window;
		}
		if(data_available > tcb->window) {
			data_available = tcb->window;
		}
	} else {
		if(data_available > tcb->mss) {
			/* If the remote host has a zero window, we probe
		 	* it by sending a single data packet.
		 	*
		 	* Quoting RFC 793, p. 42:
		 	* "The sending TCP must regularly retransmit to the receiving TCP even when
		 	*  the window is zero. [...] This retransmission is essential to guarantee
		 	*  that when either TCP has a zero window the re-opening of the window will
		 	*  be reliably reported to the other."
		 	*/
			data_available = tcb->mss;
		}
	}

	uint16_t data_pos = tcb->send_next;
	bool success = true;

	data_available -= data_pos;
	packet_seq += data_pos;
	do {
		uint16_t packet_data_len = 0;

		if(send_data) {
			/* Peek data from transmit queue and put
			 * it into the packet.
			 */
			packet_data_len = packet_data_max;
			if(packet_data_len >= data_available)
			{
				packet_data_len = data_available;

				/* The following is a hack which prevents ping-pong
				 * timing when the remote host uses a delayed-ack
				 * algorithm. If we are sending a single packet,
				 * we split it into two halves, which effectively
				 * disables delayed ACKs at the remote host.
				 */
				if(!data_pos && packet_data_len > 1) {
					//packet_data_len /= 2;
				}
			}

			tcp_queue_peek_tx( tcb->queue, packet_data, data_pos, packet_data_len );

			if(packet_data_len > 0) {
				/* start retransmission timer */
				tcb->timeout = tcb->rto << (tcb->retx > 4 ? 4 : tcb->retx); /* exponential backoff */
				if(tcb->timeout < 1) {
					tcb->timeout = 1;
				}
			}
		}

		/* set sequence number */
		packet_header->seq = hton32(packet_seq);

		/* ensure FIN is set only if we are sending the last data byte */
		if(data_pos + packet_data_len < tcp_queue_used_tx(tcb->queue))
			packet_header->flags = flags & ~TCP_FLAG_FIN;
		else
			packet_header->flags = flags;

		/* calculate header checksum */
		packet_header->checksum = hton16(tcp_calc_checksum(tcb->ip, packet_header, packet_header_size + packet_data_len));

		/* transmit packet */
		success = success && ip_send_packet(tcb->ip,
											IP_PROTOCOL_TCP,
											packet_header_size + packet_data_len
										   );

		if( success ) {
			packet_seq += packet_data_len;
			data_pos += packet_data_len;
			data_available -= packet_data_len;
		}

	} while(success && send_data && data_available > 0);

	tcb->send_next = data_pos;

	return success;
}

/**
 * Send a TCP reset packet.
 *
 * A reset package immediately closes a connection.
 *
 * \param[in] ip_destination The IP address of the destination host.
 * \param[in] packet A pointer to the buffer which contains the TCP packet to which the reset package should be an response.
 * \param[in] packet_len The packet length in bytes of the packet in \a packet.
 * \returns \c true if the reset packet has successfully been sent, \c false on failure.
 */
bool tcp_send_rst(const uint8_t* ip_destination, const struct tcp_header* packet, uint16_t packet_len)
{
	if(!packet)
		return false;
	if(packet->flags & TCP_FLAG_RST)
		return false;

	struct tcp_header* packet_header = (struct tcp_header*) ip_get_buffer();

	/* prepare packet header */
	memset(packet_header, 0, sizeof(*packet_header));
	packet_header->port_source = packet->port_destination;
	packet_header->port_destination = packet->port_source;
	packet_header->offset = (sizeof(*packet_header) / 4) << 4;

	if(packet->flags & TCP_FLAG_ACK)
	{
		packet_header->flags = TCP_FLAG_RST;
		packet_header->seq = packet->ack;
	}
	else
	{
		uint32_t ack = ntoh32(packet->seq) + packet_len - (packet->offset >> 4) * 4;
		if(packet->flags & TCP_FLAG_SYN)
			++ack;

		packet_header->flags = TCP_FLAG_RST | TCP_FLAG_ACK;
		packet_header->ack = hton32(ack);
	}

	/* calculate header checksum */
	packet_header->checksum = hton16(tcp_calc_checksum(ip_destination, packet_header, sizeof(*packet_header)));

	/* transmit packet */
	return ip_send_packet(ip_destination,
						  IP_PROTOCOL_TCP,
						  sizeof(*packet_header)
						 );
}
