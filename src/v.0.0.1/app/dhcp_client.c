
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../app/dhcp_client.h"
#include "../net/net.h"
#include "../net/udp.h"
#include "../sys/timer.h"

#include <stdbool.h>
#include <string.h>

/**
 * \addtogroup app
 *
 * @{
 */
/**
 * \addtogroup app_dhcp_client DHCP client
 * A DHCP client for automatically retrieving the network configuration when the link goes up.
 *
 * The client provides some events through a callback function. These events inform the application
 * whenever e.g. the network has been successfully configured or the DHCP lease has expired.
 *
 * @{
 */
/**
 * \file
 * DHCP client implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct dhcp_header
{
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint8_t ciaddr[4];
	uint8_t yiaddr[4];
	uint8_t siaddr[4];
	uint8_t giaddr[4];
	uint8_t chaddr[16];
	char sname[64];
	char file[128];
	uint32_t cookie;
};

enum dhcp_state
{
	DHCP_STATE_INIT = 0,
	DHCP_STATE_SELECTING,
	DHCP_STATE_REQUESTING,
	DHCP_STATE_BOUND,
	DHCP_STATE_REBINDING
};

struct dhcp_client_state
{
	enum dhcp_state state;
	dhcp_client_callback callback;
	int socket;
	int timer;
	uint8_t ip_server[4];
	uint8_t ip_local[4];
	uint8_t ip_netmask[4];
	uint8_t ip_gateway[4];
	uint32_t time_rebind;
	uint8_t retries;
};

#define DHCP_CLIENT_TIMEOUT 4000
#define DHCP_CLIENT_RETRIES 3
#define DHCP_CLIENT_XID 0x55059934

#define DHCP_OP_BOOTREQUEST 0x01
#define DHCP_OP_BOOTREPLY 0x02
#define DHCP_HTYPE_ETHERNET 0x01
#define DHCP_HLEN_ETHERNET 6
#define DHCP_FLAG_BROADCAST 0x8000
#define DHCP_COOKIE 0x63825363

#define DHCP_OPTION_PAD 0x00
#define DHCP_OPTION_END 0xff
#define DHCP_OPTION_NETMASK 0x01
#define DHCP_OPTION_ROUTER 0x03
#define DHCP_OPTION_TIMESERVER 0x04
#define DHCP_OPTION_DNS 0x06
#define DHCP_OPTION_REQUESTIP 0x32
#define DHCP_OPTION_TIMELEASE 0x33
#define DHCP_OPTION_MESSAGETYPE 0x35
#define DHCP_OPTION_SERVERID 0x36
#define DHCP_OPTION_PARAMREQUEST 0x37
#define DHCP_OPTION_TIMERENEW 0x3a
#define DHCP_OPTION_TIMEREBIND 0x3b

#define DHCP_OPTION_LEN_NETMASK 4
#define DHCP_OPTION_LEN_REQUESTIP 4
#define DHCP_OPTION_LEN_TIMELEASE 4
#define DHCP_OPTION_LEN_MESSAGETYPE 1
#define DHCP_OPTION_LEN_SERVERID 4
#define DHCP_OPTION_LEN_TIMERENEW 4
#define DHCP_OPTION_LEN_TIMEREBIND 4

#define DHCP_MESSAGETYPE_DHCPDISCOVER 1
#define DHCP_MESSAGETYPE_DHCPOFFER 2
#define DHCP_MESSAGETYPE_DHCPREQUEST 3
#define DHCP_MESSAGETYPE_DHCPDECLINE 4
#define DHCP_MESSAGETYPE_DHCPACK 5
#define DHCP_MESSAGETYPE_DHCPNACK 6
#define DHCP_MESSAGETYPE_DHCPRELEASE 7

static void dhcp_client_timeout(int timer);
static void dhcp_client_incoming(int socket, uint8_t* data, uint16_t data_len);

static uint8_t* dhcp_client_packet_add_msgtype(uint8_t* options, uint8_t type);
static uint8_t* dhcp_client_packet_add_params(uint8_t* options);
static uint8_t* dhcp_client_packet_add_server(uint8_t* options);
static uint8_t* dhcp_client_packet_add_ip(uint8_t* options);
static uint8_t* dhcp_client_packet_add_end(uint8_t* options);

static bool dhcp_client_send();
static uint8_t dhcp_client_parse(const struct dhcp_header* packet);

static struct dhcp_client_state state;

/**
 * Starts the DHCP client.
 *
 * When the client was started successfully, DHCP requests are being broadcasted
 * on the network.
 *
 * \param[in] callback A function pointer which is called for every event the client generates.
 * \returns \c true if the client was started successfully, \c false on failure.
 */
bool dhcp_client_start(dhcp_client_callback callback)
{
	if(state.state != DHCP_STATE_INIT || !callback)
		return false;

	memset(&state, 0, sizeof(state));

	/* allocate timer */
	state.timer = timer_alloc(dhcp_client_timeout, 0);
	if( !timer_valid(state.timer) ) {
		return false;
	}

	/* allocate udp socket */
	state.socket = udp_socket_alloc(dhcp_client_incoming);
	if( !udp_socket_valid(state.socket) ) {
		timer_free(state.timer);
		return false;
	}

	/* reset network configuration */

	ip_set_address(state.ip_local);
	ip_set_netmask(state.ip_netmask);
	ip_set_gateway(state.ip_gateway);

	/* start on the next timeout */
	timer_set(state.timer, 1000);
	state.state = DHCP_STATE_INIT;
	state.callback = callback;
	state.retries = 0;

	return true;
}

/**
 * Aborts all ongoing DHCP actions and terminates the client.
 *
 * \note The network configuration is not changed by calling this function. You
 *	   may want to deconfigure the network interface to no longer occupy an
 *	   IP address managed by the DHCP server.
 */
void dhcp_client_abort()
{
	if( DHCP_STATE_INIT == state.state )
		return;

	timer_free(state.timer);
	udp_socket_free(state.socket);
	state.state = DHCP_STATE_INIT;
}

/**
 * Timeout function for handling lease and request timeouts.
 */
void dhcp_client_timeout(int timer)
{
	switch(state.state)
	{
		case DHCP_STATE_INIT:
		{
			/* state transition */
			state.state = DHCP_STATE_SELECTING;
			state.retries = 0xff;

			/* fall through */
		}
		case DHCP_STATE_SELECTING:
		{
			if(++state.retries > DHCP_CLIENT_RETRIES)
			{
				dhcp_client_abort();
				state.callback(DHCP_CLIENT_EVT_TIMEOUT);
				return;
			}

			/* resend discover */
			if(!dhcp_client_send())
			{
				dhcp_client_abort();
				state.callback(DHCP_CLIENT_EVT_ERROR);
				return;
			}

			timer_set(state.timer, DHCP_CLIENT_TIMEOUT << state.retries);

			break;
		}
		case DHCP_STATE_BOUND:
		{
			/* state transition */
			state.state = DHCP_STATE_REBINDING;
			state.retries = 0xff;

			/* fall through */
		}
		case DHCP_STATE_REQUESTING:
		case DHCP_STATE_REBINDING:
		{
			if(++state.retries > DHCP_CLIENT_RETRIES)
			{
				enum dhcp_client_event evt = DHCP_CLIENT_EVT_TIMEOUT;
				if(state.state == DHCP_STATE_REBINDING)
					evt = DHCP_CLIENT_EVT_LEASE_EXPIRING;

				dhcp_client_abort();
				state.callback(evt);
				return;
			}

			/* resend dhcp request */
			if(!dhcp_client_send())
			{
				dhcp_client_abort();
				state.callback(DHCP_CLIENT_EVT_ERROR);
				return;
			}

			timer_set(state.timer, DHCP_CLIENT_TIMEOUT << state.retries);

			break;
		}
	}
}

/**
 * UDP callback function which handles all incoming DHCP packets.
 */
void dhcp_client_incoming(int socket, uint8_t* data, uint16_t data_len)
{
	const struct dhcp_header* header = (struct dhcp_header*) data;

	/* accept responses only */
	if(header->op != DHCP_OP_BOOTREPLY)
		return;

	/* accept responses with our id only */
	if(header->xid != HTON32(DHCP_CLIENT_XID))
		return;

	/* accept responses with correct cookie only */
	if(header->cookie != HTON32(DHCP_COOKIE))
		return;

	/* accept responses with our hardware address only */
	if(memcmp(header->chaddr, ethernet_get_mac(), 6) != 0)
		return;

	/* parse dhcp packet options */
	uint8_t msg_type = dhcp_client_parse(header);
	if(msg_type == 0)
		return;

	switch(state.state)
	{
		case DHCP_STATE_INIT:
		case DHCP_STATE_BOUND:
		{
			break;
		}
		case DHCP_STATE_SELECTING:
		{
			/* discard anything which is not an offer */
			if(msg_type != DHCP_MESSAGETYPE_DHCPOFFER)
				return;

			/* state transition */
			timer_set(state.timer, DHCP_CLIENT_TIMEOUT);
			state.state = DHCP_STATE_REQUESTING;
			state.retries = 0;

			/* generate dhcp reply */
			if(!dhcp_client_send())
			{
				dhcp_client_abort();
				state.callback(DHCP_CLIENT_EVT_ERROR);
				return;
			}

			break;
		}
		case DHCP_STATE_REQUESTING:
		case DHCP_STATE_REBINDING:
		{
			/* ignore other/resent offers */
			if(msg_type == DHCP_MESSAGETYPE_DHCPOFFER)
				return;

			/* restart from scratch if we receive something which is not an ack */
			if(msg_type != DHCP_MESSAGETYPE_DHCPACK)
			{
				enum dhcp_client_event evt = DHCP_CLIENT_EVT_LEASE_DENIED;
				if(state.state == DHCP_STATE_REBINDING)
					evt = DHCP_CLIENT_EVT_LEASE_EXPIRED;

				dhcp_client_abort();
				state.callback(evt);
				return;
			}

			/* configure interface */
			ip_set_address(state.ip_local);
			ip_set_netmask(state.ip_netmask);
			ip_set_gateway(state.ip_gateway);

			/* wait for the lease to expire */
			timer_set(state.timer, state.time_rebind * 1024); /* 1024 approximates 1000 and saves code */
			state.state = DHCP_STATE_BOUND;
			state.callback(DHCP_CLIENT_EVT_LEASE_ACQUIRED);

			break;
		}
	}
}

/**
 * Adds a message type option to the given DHCP packet.
 *
 * \param[in] options A pointer to the packet's end.
 * \param[in] type The message type to add to the packet.
 * \returns A pointer to the packet's new end.
 */
uint8_t* dhcp_client_packet_add_msgtype(uint8_t* options, uint8_t type)
{
	*options++ = DHCP_OPTION_MESSAGETYPE;
	*options++ = DHCP_OPTION_LEN_MESSAGETYPE;
	*options++ = type;

	return options;
}

/**
 * Adds a parameter request option to the given DHCP packet.
 *
 * The option requests the network parameters needed from the DHCP server.
 *
 * \param[in] options A pointer to the packet's end.
 * \returns A pointer to the packet's new end.
 */
uint8_t* dhcp_client_packet_add_params(uint8_t* options)
{
	*options++ = DHCP_OPTION_PARAMREQUEST;
	*options++ = 2;
	*options++ = DHCP_OPTION_NETMASK;
	*options++ = DHCP_OPTION_ROUTER;

	return options;
}

/**
 * Adds the DHCP server IP address as an option to the given DHCP packet.
 *
 * \param[in] options A pointer to the packet's end.
 * \returns A pointer to the packet's new end.
 */
uint8_t* dhcp_client_packet_add_server(uint8_t* options)
{
	*options++ = DHCP_OPTION_SERVERID;
	*options++ = DHCP_OPTION_LEN_SERVERID;
	memcpy(options, state.ip_server, sizeof(state.ip_server));
	options += sizeof(state.ip_server);

	return options;
}

/**
 * Adds our anticipated IP address as an option to the given DHCP packet.
 *
 * \param[in] options A pointer to the packet's end.
 * \returns A pointer to the packet's new end.
 */
uint8_t* dhcp_client_packet_add_ip(uint8_t* options)
{
	*options++ = DHCP_OPTION_REQUESTIP;
	*options++ = DHCP_OPTION_LEN_REQUESTIP;
	memcpy(options, state.ip_local, sizeof(state.ip_local));
	options += sizeof(state.ip_local);

	return options;
}

/**
 * Adds an option to the given DHCP packet marking the end of the option section.
 *
 * \param[in] options A pointer to the packet's end.
 * \returns A pointer to the packet's new end.
 */
uint8_t* dhcp_client_packet_add_end(uint8_t* options)
{
	*options++ = DHCP_OPTION_END;

	return options;
}

/**
 * Generates and sends a DHCP packet appropriate for the current client state.
 *
 * \returns \c true on success, \c false on failure.
 */
bool dhcp_client_send()
{
	/* generate dhcp header */
	struct dhcp_header* packet = (struct dhcp_header*) udp_get_buffer();
	memset(packet, 0, sizeof(*packet));
	packet->op = DHCP_OP_BOOTREQUEST;
	packet->htype = DHCP_HTYPE_ETHERNET;
	packet->hlen = DHCP_HLEN_ETHERNET;
	packet->xid = HTON32(DHCP_CLIENT_XID);
	packet->flags = HTON16(DHCP_FLAG_BROADCAST);
	packet->cookie = HTON32(DHCP_COOKIE);
	memcpy(packet->ciaddr, ip_get_address(), 4);
	memcpy(packet->chaddr, ethernet_get_mac(), 6);

	/* determine message type from current state */
	uint8_t msg_type;
	if(state.state == DHCP_STATE_SELECTING)
		msg_type = DHCP_MESSAGETYPE_DHCPDISCOVER;
	else if(state.state == DHCP_STATE_REQUESTING ||
			state.state == DHCP_STATE_REBINDING
		   )
		msg_type = DHCP_MESSAGETYPE_DHCPREQUEST;
	else
		return false;

	/* append dhcp options */
	uint8_t* options_end = (uint8_t*) (packet + 1);
	options_end = dhcp_client_packet_add_msgtype(options_end, msg_type);
	options_end = dhcp_client_packet_add_params(options_end);
	if(state.state == DHCP_STATE_REQUESTING)
	{
		options_end = dhcp_client_packet_add_ip(options_end);
		options_end = dhcp_client_packet_add_server(options_end);
	}
	options_end = dhcp_client_packet_add_end(options_end);

	/* send dhcp packet */
	if(!udp_bind_local(state.socket, 68))
		return false;
	if(!udp_bind_remote(state.socket, 0, 67))
		return false;
	return udp_send(state.socket, options_end - (uint8_t*) packet);
}

/**
 * Parses the given DHCP server packet.
 *
 * \param[in] packet The packet received from the DHCP server.
 * \returns The message type of the packet on success, \c 0 on failure.
 */
uint8_t dhcp_client_parse(const struct dhcp_header* packet)
{
	if(!packet)
		return 0;

	/* save ip address proposed by the server */
	memcpy(state.ip_local, packet->yiaddr, sizeof(state.ip_local));

	/* parse options */
	uint8_t msg_type = 0;
	const uint8_t* opts = (const uint8_t*) (packet + 1);
	while(1)
	{
		const uint8_t* opt_data = opts + 2;
		uint8_t opt_type = *opts;
		uint8_t opt_len = *(opts + 1);

		if(opt_type == DHCP_OPTION_PAD)
		{
			++opts;
		}
		else if(opt_type == DHCP_OPTION_END)
		{
			break;
		}
		else if(opt_type == DHCP_OPTION_NETMASK && opt_len == DHCP_OPTION_LEN_NETMASK)
		{
			memcpy(state.ip_netmask, opt_data, sizeof(state.ip_netmask));
			opts += DHCP_OPTION_LEN_NETMASK + 2;
		}
		else if(opt_type == DHCP_OPTION_ROUTER && opt_len / 4 > 0)
		{
			memcpy(state.ip_gateway, opt_data, sizeof(state.ip_gateway));
			opts += opt_len + 2;
		}
		else if(opt_type == DHCP_OPTION_TIMELEASE && opt_len == DHCP_OPTION_LEN_TIMELEASE)
		{
			state.time_rebind = ntoh32(*((const uint32_t*) opt_data));
			state.time_rebind -= state.time_rebind / 8;
			opts += DHCP_OPTION_LEN_TIMELEASE + 2;
		}
		else if(opt_type == DHCP_OPTION_MESSAGETYPE && opt_len == DHCP_OPTION_LEN_MESSAGETYPE)
		{
			msg_type = *opt_data;
			opts += DHCP_OPTION_LEN_MESSAGETYPE + 2;
		}
		else if(opt_type == DHCP_OPTION_SERVERID && opt_len == DHCP_OPTION_LEN_SERVERID)
		{
			memcpy(state.ip_server, opt_data, sizeof(state.ip_server));
			opts += DHCP_OPTION_LEN_SERVERID + 2;
		}
		else
		{
			opts += opt_len + 2;
		}
	}

	return msg_type;
}

/**
 * @}
 * @}
 */

