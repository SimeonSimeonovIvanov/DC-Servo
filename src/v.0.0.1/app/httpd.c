
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "httpd.h"
#include "httpd_config.h"
#include "httpd_modules.h"
#include "httpd_session.h"

#include "../net/tcp.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

/**
 * \addtogroup app Applications
 * Different example applications based on the \link net network interface \endlink.
 *
 * @{
 */
/**
 * \addtogroup app_httpd HTTP server
 * A small web server with GET and POST support.
 *
 * The HTTP server answers incoming requests by executing built-in modules. The request
 * forms a so-called session and the modules are asked wether they can handle the
 * session. If so, the module is responsible for sending an appropriate response back to
 * the HTTP client. If a module is unable to handle the session, the next module is asked.
 *
 * Once a module is found it is bound to the session and reexecuted whenever some of
 * the data has been sent, so that the module can continue sending data.
 *
 * When the complete response was generated, the module has to close the session and the
 * HTTP connection will be shut down.
 *
 * @{
 */
/**
 * \file
 * HTTP server main implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

static void httpd_connection_handler(int socket, enum tcp_event event);

static struct httpd_session* httpd_new_session();
static struct httpd_session* httpd_session_with_socket(int socket);

static void httpd_parse_request(struct httpd_session* session);
static void httpd_tokenize_line(char* str);

/**
 * An array of modules available to handle a session.
 */
static httpd_module_callback httpd_modules[] =
{
	httpd_module_get_status_xml_callback,
	httpd_module_do_togle_callback,

	httpd_module_dir_callback,
	httpd_module_file_callback,

	httpd_module_auth_callback,
	httpd_module_root_callback,
	httpd_module_demo_arg_callback
};

static struct httpd_session httpd_sessions[HTTPD_MAX_SESSIONS];

/**
 * Starts the HTTP server.
 *
 * The server starts listening on the specified TCP port.
 *
 * \param[in] port The port on which to listen for HTTP requests.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_init(uint16_t port)
{
	int listener = tcp_socket_alloc(httpd_connection_handler);
	return tcp_listen(listener, port);
}

/**
 * Handles the incoming connections.
 *
 * Creates a session for every incoming request and searches a module
 * which will handle the session.
 *
 * \param[in] socket The identifier of the socket on which the \a event occured.
 * \param[in] event The identifier of the event which was generated for the socket.
 */
void httpd_connection_handler(int socket, enum tcp_event event)
{
	struct httpd_session* session = httpd_session_with_socket(socket);

	switch(event)
	{
		case TCP_EVT_ERROR:
		case TCP_EVT_RESET:
		case TCP_EVT_TIMEOUT:
		case TCP_EVT_CONN_CLOSED:
		{
			if(session)
			{
				if(session->module)
					session->module(session, HTTPD_MODULE_REASON_CLEANUP);

				session->method = HTTPD_METHOD_UNUSED;
			}

			tcp_disconnect(socket);
			tcp_socket_free(socket);
			break;
		}
		case TCP_EVT_CONN_INCOMING:
		{
			tcp_accept(socket, httpd_connection_handler);
			break;
		}
		case TCP_EVT_CONN_IDLE:
		case TCP_EVT_DATA_SENT:
		case TCP_EVT_DATA_RECEIVED:
		{
			if(session)
			{
				if(!session->module)
					break;

				/* Either the client is sending a request body for an existing session
				 * or outgoing data has been sent. Both cases will be handled by the
				 * session.
				 */
				session->module(session, HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE);
				break;
			}

			session = httpd_new_session();
			if(!session)
			{
				/* There is no unused session slot available. The session will be
				 * tried to get started when an idle event is generated on the
				 * socket.
				 */
				break;
			}

			/* setup session */
			memset(session, 0, sizeof(*session));
			session->socket = socket;

			/* parse and handle the request */
			httpd_parse_request(session);
			switch(session->method)
			{
				case HTTPD_METHOD_GET:
				case HTTPD_METHOD_HEAD:
				case HTTPD_METHOD_POST:
				{
					for(uint8_t i = 0; i < sizeof(httpd_modules) / sizeof(httpd_modules[0]); ++i) {
						if( true == httpd_modules[i](session, HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST) ) {
							session->module = httpd_modules[i];
							session->module(session, HTTPD_MODULE_REASON_HANDLE_REQUEST);
							break;
						}
					}
					if(!session->module) {
						httpd_session_write_error(session, 503);
						httpd_session_close(session);
					}
					break;
				}

				case HTTPD_METHOD_INCOMPLETE:
				{
					session->method = HTTPD_METHOD_UNUSED;
					break;
				}
				case HTTPD_METHOD_INVALID:
				{
					httpd_session_write_error(session, 400);
					httpd_session_close(session);
					break;
				}
				case HTTPD_METHOD_UNKNOWN:
				default:
				{
					httpd_session_write_error(session, 501);
					httpd_session_close(session);
					break;
				}
			}

			break;
		}
		default:
			break;
	}
}

/**
 * Searches for an unused session within the session array.
 *
 * \returns A pointer to an unused session on success, \c 0 on failure.
 */
struct httpd_session* httpd_new_session()
{
	for(uint8_t i = 0; i < sizeof(httpd_sessions) / sizeof(httpd_sessions[0]); ++i)
	{
		if(httpd_sessions[i].method == HTTPD_METHOD_UNUSED)
			return &httpd_sessions[i];
	}

	return 0;
}

/**
 * Searches for an active session with a known socket identifier.
 *
 * \returns A pointer to the found session on success, \c 0 on failure.
 */
struct httpd_session* httpd_session_with_socket(int socket)
{
	for(uint8_t i = 0; i < sizeof(httpd_sessions) / sizeof(httpd_sessions[0]); ++i)
	{
		if(httpd_sessions[i].method == HTTPD_METHOD_UNUSED)
			continue;
		if(httpd_sessions[i].socket == socket)
			return &httpd_sessions[i];
	}

	return 0;
}

/**
 * Parses incoming HTTP requests and extracts session parameters.
 *
 * \param[in,out] session The session structure which to fill with the session parameters.
 */
void httpd_parse_request(struct httpd_session* session)
{
	if(!session)
		return;

	session->method = HTTPD_METHOD_INCOMPLETE;

	int socket = session->socket;
	uint16_t available = tcp_buffer_used_rx(socket);
	char* buffer = (char*) tcp_buffer_rx(socket);

	/* skip all whitespace */
	while(available > 0)
	{
		if(*buffer != '\r' &&
		   *buffer != '\n' &&
		   *buffer != '\011' &&
		   *buffer != ' '
		  )
			break;

		++buffer;
		--available;
	}
	session->uri = buffer;

	/* check for a complete request */
	session->headers = 0;
	char* body = 0;
	for(; available >= 4; --available, ++buffer)
	{
		if(buffer[0] == '\r' && buffer[1] == '\n')
		{
			if(!session->headers)
				session->headers = buffer + 2;

			if(buffer[2] == '\r' && buffer[3] == '\n')
			{
				body = buffer + 4;
				available -= 4;

				*(session->headers - 2) = '\0';
				buffer[2] = '\0';
				buffer[3] = '\0';

				break;
			}
		}
	}

	if(tcp_buffer_space_rx(socket) < 1)
	{
		/* request too large */
		session->method = HTTPD_METHOD_INVALID;
		return;
	}

	if(!body)
		/* request headers not yet fully received */
		return;

	/* ok, the complete request has arrived */
	session->method = HTTPD_METHOD_INVALID;

	/* split the request line */
	httpd_tokenize_line(session->uri);

	/* check for kind of request */
	char* token = session->uri;
	enum httpd_method request_method = HTTPD_METHOD_INVALID;
	if(strlen(token) < 1)
		return;
	if(strcmp_P(token, PSTR("GET")) == 0)
		request_method = HTTPD_METHOD_GET;
	else if(strcmp_P(token, PSTR("HEAD")) == 0)
		request_method = HTTPD_METHOD_HEAD;
	else if(strcmp_P(token, PSTR("POST")) == 0)
		request_method = HTTPD_METHOD_POST;
	else
		request_method = HTTPD_METHOD_UNKNOWN;

	/* check for resource */
	token += strlen(token) + 1;
	if(strlen(token) < 1)
		return;
	session->uri = token;

	/* check for protocol */
	token += strlen(token) + 1;
	if(strcmp_P(token, PSTR("HTTP/1.1")) == 0)
		session->flags = HTTPD_CLIENT_11;
	else if(strcmp_P(token, PSTR("HTTP/1.0")) == 0)
		session->flags = HTTPD_CLIENT_10;
	else if(strncmp_P(token, PSTR("HTTP/"), 5) == 0)
		session->flags = HTTPD_CLIENT_UNKNOWN;
	else
		return;

	/* split headers */
	token = strchr(session->headers, '\r');
	while(token)
	{
		/* We split the headers line by line. Each line
		 * will be terminated with a double null byte.
		 * The last header line gets terminated with four
		 * null bytes.
		 */
		token[0] = '\0';
		token[1] = '\0';
		token = strchr(token + 2, '\r');
	}

	/* search for arguments */
	token = strchr(session->uri, '?');
	if(token)
		*token = '\0';

	if(request_method == HTTPD_METHOD_POST)
	{
		session->arguments = body;
	}
	else
	{
		if(token)
			session->arguments = token + 1;
		else
			session->arguments = session->uri + strlen(session->uri);

		session->arguments_end = session->arguments + strlen(session->arguments);
	}

	session->method = request_method;
}

/**
 * Devides a string into a chain of words separated by nulls.
 *
 * \param[in,out] str The string which to tokenize.
 */
void httpd_tokenize_line(char* str)
{
	while(*str)
	{
		if(*str == ' ')
			*str = '\0';

		++str;
	}
}

/**
 * @}
 * @}
 */

