/*
 * FreeModbus Libary: Win32 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttcp.c,v 1.2 2006/06/26 19:24:07 wolti Exp $
 */

/*
 * Design Notes:
 *
 * The xMBPortTCPInit function allocates a socket and binds the socket to
 * all available interfaces ( bind with INADDR_ANY ). In addition it
 * creates an array of event objects which is used to check the state of
 * the clients. On event object is used to handle new connections or
 * closed ones. The other objects are used on a per client basis for
 * processing.
 */

#include <stdio.h>
#include <string.h>

#include "port.h"
#include "porttcp.h"
#include "../../net/tcp.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- MBAP Header --------------------------------------*/
#define MB_TCP_UID				6
#define MB_TCP_LEN				4
#define MB_TCP_FUNC				7

/* ----------------------- Defines  -----------------------------------------*/
#define MB_TCP_DEFAULT_PORT		502 /* TCP listening port. */
#define MB_TCP_BUF_SIZE			( 256 + 7 ) /* Must hold a complete Modbus TCP frame. */

#define EV_CONNECTION			0
#define EV_CLIENT				1
#define EV_NEVENTS				EV_CLIENT + 1

/* ----------------------- Static variables ---------------------------------*/
//volatile static UCHAR aucTCPBuf[ MB_TCP_BUF_SIZE ];
//volatile static int16_t usTCPBufPos;

static struct mbtcp_session mbtcp_sessions[ 5 ];

volatile static int tcp_listener_socket;

/* ----------------------- External functions -------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
static void modbus_connection_handler(int socket, enum tcp_event event);

static struct mbtcp_session* mbtcp_new_session( void );
static struct mbtcp_session* mbtcp_session_with_socket( int socket );

/* ----------------------- Begin implementation -----------------------------*/

BOOL
xMBTCPPortInit( USHORT usTCPPort ) // ???
{
	int listener;
	USHORT usPort;

	if( !usTCPPort ) {
        usPort = MB_TCP_DEFAULT_PORT;
    } else {
		usPort = ( USHORT ) usTCPPort;
    }

	listener = tcp_socket_alloc( modbus_connection_handler );
	tcp_listen( listener, usTCPPort + 0 );

	//listener = tcp_socket_alloc( modbus_connection_handler );
	//tcp_listen( listener, usTCPPort + 1 );

	//listener = tcp_socket_alloc( modbus_connection_handler );
	//tcp_listen( listener, usTCPPort + 2);

	return true;
}

void
vMBTCPPortClose( void ) // ???
{
	if( mbtcp_session_with_socket( tcp_listener_socket ) ) {
		tcp_disconnect( tcp_listener_socket );
		tcp_socket_free( tcp_listener_socket );
	}
}

void
vMBTCPPortDisable( void )
{
}

BOOL
xMBTCPPortGetRequest( UCHAR ** ppucMBTCPFrame, USHORT * usTCPLength )
{
	struct mbtcp_session* session = mbtcp_session_with_socket( tcp_listener_socket );

	if( !session ) {
		return FALSE;
	}

	*ppucMBTCPFrame = (UCHAR*)&session->aucTCPBuf[0];
	*usTCPLength = session->usTCPBufLen;

	/* Reset the buffer. */
    session->usTCPBufLen = 0;

	return TRUE;
}

BOOL
xMBTCPPortSendResponse( const UCHAR * pucMBTCPFrame, USHORT usTCPLength ) // ???
{	
	struct mbtcp_session* session = mbtcp_session_with_socket( tcp_listener_socket );
	BOOL bFrameSent = FALSE;

	if( session && tcp_listener_socket >= 0) {
		if( usTCPLength == tcp_write( tcp_listener_socket, pucMBTCPFrame, (uint16_t)usTCPLength ) ) {
            /* Make sure data gets sent immediately. */
            bFrameSent = TRUE;
        } else {
            /* Drop the connection in case of an write error. */
            vMBTCPPortClose();
        }
    }

	return bFrameSent;
}

static void modbus_connection_handler(int socket, enum tcp_event event)
{
	struct mbtcp_session* session = mbtcp_session_with_socket( socket );

	switch(event) {
	case TCP_EVT_ERROR:
	case TCP_EVT_RESET:
	case TCP_EVT_TIMEOUT:
	case TCP_EVT_CONN_CLOSED:

		if( session ) {
			session->method = MBTCP_METHOD_UNUSED;
		}

		tcp_disconnect( socket );
		tcp_socket_free( socket );
	 break;

	case TCP_EVT_CONN_INCOMING:
		if( !session ) {
			if( ( session = mbtcp_new_session() ) ) {

				session->method = MBTCP_METHOD_USED;
				session->socket = tcp_listener_socket = socket;
			}
		}

		tcp_accept( socket, modbus_connection_handler );
	 break;

	//case TCP_EVT_CONN_IDLE: // ???
	//case TCP_EVT_DATA_SENT: // ???
	case TCP_EVT_DATA_RECEIVED: {

		if( !session ) {
			return;
		}

		session->usTCPBufLen = tcp_buffer_used_rx( socket );

		if( session->usTCPBufLen <= 0 ) {
			tcp_disconnect( socket );
			return;
		}

		if( session->usTCPBufLen >= MB_TCP_BUF_SIZE ) {
			tcp_disconnect( socket );
			return;
		}

		if( tcp_read( socket, (uint8_t*)session->aucTCPBuf, MB_TCP_BUF_SIZE ) <= 0 ) {
			tcp_disconnect( socket );
			return;
		}

		if( session->aucTCPBuf[3] || session->aucTCPBuf[2] ) {
			tcp_disconnect( socket );
			return;
		}

		/* Length is a byte count of Modbus PDU (function code + data) and the unit identifier. */
		USHORT usLength;
		usLength  = session->aucTCPBuf[ MB_TCP_LEN + 0 ]<<8;
		usLength |= session->aucTCPBuf[ MB_TCP_LEN + 1 ];

		/* The frame is complete. */
		if( ( MB_TCP_UID + usLength ) == session->usTCPBufLen ) {
			tcp_listener_socket = socket;
			(void)xMBPortEventPost( EV_FRAME_RECEIVED );
		} else {
			/* This can not happend because we always calculate the number of bytes
             * to receive. */
			//assert( usTCPBufPos <= ( MB_TCP_UID + usLength ) );
		}
	}
	 break;

	default: break;
    }
}

static struct mbtcp_session* mbtcp_new_session( void )
{
	for( uint8_t i = 0; i < sizeof(mbtcp_sessions) / sizeof(*mbtcp_sessions); ++i ) {
		if( MBTCP_METHOD_UNUSED == mbtcp_sessions[i].method ) {
			return &mbtcp_sessions[i];
		}
	}

	return 0;
}

static struct mbtcp_session* mbtcp_session_with_socket( int socket )
{
	for( uint8_t i = 0; i < sizeof(mbtcp_sessions) / sizeof(*mbtcp_sessions); ++i ) {

		if( MBTCP_METHOD_UNUSED == mbtcp_sessions[i].method ) {
			continue;
		}
		
		if( mbtcp_sessions[i].socket == socket) {
			return &mbtcp_sessions[i];
		}

	}

	return 0;
}
