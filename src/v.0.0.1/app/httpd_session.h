
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef HTTPD_SESSION_H
#define HTTPD_SESSION_H

#include "httpd_modules.h"

#include <stdbool.h>
#include <stdint.h>
#include <avr/pgmspace.h>

/**
 * \addtogroup app
 *
 * @{
 */
/**
 * \addtogroup app_httpd
 *
 * @{
 */
/**
 * \file
 * HTTP server session layer header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * \internal
 * The request methods supported by a session.
 */
enum httpd_method
{
    /** \internal The session structure is unused. */
    HTTPD_METHOD_UNUSED = 0,
    /** \internal The session request has not yet fully been received. */
    HTTPD_METHOD_INCOMPLETE,
    /** \internal The session request is invalid or malformed. */
    HTTPD_METHOD_INVALID,
    /** \internal The session request method is unknown/unsupported. */
    HTTPD_METHOD_UNKNOWN,
    /** \internal The session request method is GET. */
    HTTPD_METHOD_GET,
    /** \internal The session request method is HEAD. */
    HTTPD_METHOD_HEAD,
    /** \internal The session request method is POST. */
    HTTPD_METHOD_POST
};

/**
 * \internal
 * Flags to attach to a session.
 */
enum httpd_flags
{
    /** \internal The client uses an unknown HTTP version. */
    HTTPD_CLIENT_UNKNOWN = 0,
    /** \internal The client uses HTTP/1.0. */
    HTTPD_CLIENT_10 = 1,
    /** \internal The client uses HTTP/1.1. */
    HTTPD_CLIENT_11 = 2,
    /** \internal We response with the "chunked" transfer encoding. */
    HTTPD_TRANSFER_CHUNKED = 4,
    /** \internal We decoded the request parameters before. */
    HTTPD_ARGUMENTS_DECODED = 8
};

/**
 * \internal
 * The session structure.
 */
struct httpd_session
{
    /** The socket of the session connection. */
    int socket;
    /** The HTTP method used for the session. */
    uint8_t method;
    /** The flags attached to the session. */
    uint8_t flags;
    /** The module attached to the session. */
    httpd_module_callback module;
    /** The request URI. */
    char* uri;
    /** The headers of the request. */
    char* headers;
    /** The GET or POST arguments of the request. */
    char* arguments;
    /** The end of the request arguments. */
    char* arguments_end;
    /** A field freely usable by the module. */
    uintptr_t user;
};

bool httpd_session_write_status(const struct httpd_session* session, uint16_t status);
bool httpd_session_write_error(struct httpd_session* session, uint16_t error);
bool httpd_session_write_header(const struct httpd_session* session, const char* name, const char* value);
bool httpd_session_write_header_P(const struct httpd_session* session, PGM_P name, const char* value);
bool httpd_session_write_header_PP(const struct httpd_session* session, PGM_P name, PGM_P value);
bool httpd_session_begin_content(struct httpd_session* session);
bool httpd_session_begin_content_chunked(struct httpd_session* session);
bool httpd_session_end_content(struct httpd_session* session);
int16_t httpd_session_write_content(const struct httpd_session* session, const uint8_t* content, uint16_t content_length);
int16_t httpd_session_write_content_P(const struct httpd_session* session, PGM_P content);
uint8_t* httpd_session_get_write_buffer(const struct httpd_session* session);
uint16_t httpd_session_get_write_buffer_size(const struct httpd_session* session);
int16_t httpd_session_reserve_write_buffer(const struct httpd_session* session, uint16_t len);
char* httpd_session_read_header_P(const struct httpd_session* session, PGM_P name);
char* httpd_session_read_argument_P(struct httpd_session* session, PGM_P name);
bool httpd_session_close(struct httpd_session* session);

/**
 * @}
 * @}
 */

#endif

