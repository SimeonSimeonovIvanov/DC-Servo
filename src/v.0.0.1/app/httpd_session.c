
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "httpd_config.h"
#include "httpd_session.h"

#include "../net/tcp.h"

#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * HTTP server session layer implementation (license: GPLv2)
 *
 * There is session support for:
 * - Status and error reporting.
 * - Chunked transfer encoding for HTTP/1.1 clients.
 * - Sending user-defined header fields.
 * - Reading header fields of HTTP requests.
 * - Reading arguments of GET and POST requests.
 *
 * \author Roland Riegel
 */

struct httpd_status
{
    uint16_t code;
    const char* message;
};

static const char PROGMEM httpd_session_msg_200[] = "OK";
static const char PROGMEM httpd_session_msg_302[] = "Found";
static const char PROGMEM httpd_session_msg_400[] = "Bad Request";
static const char PROGMEM httpd_session_msg_401[] = "Unauthorized";
static const char PROGMEM httpd_session_msg_403[] = "Forbidden";
static const char PROGMEM httpd_session_msg_404[] = "Not Found";
static const char PROGMEM httpd_session_msg_408[] = "Request Timeout";
static const char PROGMEM httpd_session_msg_500[] = "Internal Server Error";
static const char PROGMEM httpd_session_msg_501[] = "Not Implemented";
static const char PROGMEM httpd_session_msg_503[] = "Service Unavailable";

struct httpd_status httpd_messages[] PROGMEM =
{
    { 200, httpd_session_msg_200 },
    { 302, httpd_session_msg_302 },
    { 400, httpd_session_msg_400 },
    { 401, httpd_session_msg_401 },
    { 403, httpd_session_msg_403 },
    { 404, httpd_session_msg_404 },
    { 408, httpd_session_msg_408 },
    { 500, httpd_session_msg_500 },
    { 501, httpd_session_msg_501 },
    { 503, httpd_session_msg_503 },
};

static PGM_P httpd_session_get_status_message(uint16_t status);

/**
 * \internal
 * Sends the HTTP response status line.
 *
 * \param[in] session The session for which to send the status line.
 * \param[in] status The status code to send.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_write_status(const struct httpd_session* session, uint16_t status)
{
    if(status > 999)
        return false;

    PGM_P msg = httpd_session_get_status_message(status);
    if(!msg)
        return false;

    if((session->flags & HTTPD_CLIENT_11) == HTTPD_CLIENT_11)
    {
        if(tcp_write_string_P(session->socket, PSTR("HTTP/1.1 ")) < 9)
            return false;
    }
    else
    {
        if(tcp_write_string_P(session->socket, PSTR("HTTP/1.0 ")) < 9)
            return false;
    }

    char status_buffer[5];
    sprintf_P(status_buffer, PSTR("%03d "), status);
    if(tcp_write(session->socket, (uint8_t*) status_buffer, 4) < 1)
        return false;

    if(tcp_write_string_P(session->socket, msg) < 1)
        return false;
    if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
        return false;
    if(!httpd_session_write_header_PP(session, PSTR("Server"), PSTR(HTTPD_NAME)))
        return false;

    return true;
}

/**
 * \internal
 * Answers the session request with an error message.
 *
 * This function sends a complete HTTP response, including a status line,
 * some headers and a body containing the message corresponding to the
 * given error code.
 *
 * \note The session is \b not closed after the message has been sent.
 *
 * \param[in] session The session for which to send the error message.
 * \param[in] error The error code which defines the message to send.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_write_error(struct httpd_session* session, uint16_t error)
{
    if(!httpd_session_write_status(session, error))
        return false;

    if(!httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/html")))
        return false;

    PGM_P msg = httpd_session_get_status_message(error);
    if(!msg)
        return false;

    if(!httpd_session_begin_content_chunked(session))
        return false;

    char error_buffer[7];
    sprintf_P(error_buffer, PSTR("%03d - "), error);
    if(httpd_session_write_content(session, (uint8_t*) error_buffer, 6) < 6)
        return false;
    if(httpd_session_write_content_P(session, msg) < 0)
        return false;

    return httpd_session_end_content(session);
}

/**
 * \internal
 * Writes a header line to the session connection.
 *
 * \param[in] session The session for which to send the header.
 * \param[in] name The name of the header which to send.
 * \param[in] value The header value.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_write_header(const struct httpd_session* session, const char* name, const char* value)
{
    if(tcp_write(session->socket, (uint8_t*) name, strlen(name)) < 1)
        return false;
    if(tcp_write_string_P(session->socket, PSTR(": ")) < 2)
        return false;
    if(tcp_write(session->socket, (uint8_t*) value, strlen(value)) < 0)
        return false;
    if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
        return false;

    return true;
}

/**
 * \internal
 * Writes a header line to the session connection.
 *
 * \param[in] session The session for which to send the header.
 * \param[in] name The name of the header which to send. This string must reside in program memory.
 * \param[in] value The header value.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_write_header_P(const struct httpd_session* session, PGM_P name, const char* value)
{
    if(tcp_write_string_P(session->socket, name) < 1)
        return false;
    if(tcp_write_string_P(session->socket, PSTR(": ")) < 2)
        return false;
    if(tcp_write(session->socket, (uint8_t*) value, strlen(value)) < 0)
        return false;
    if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
        return false;

    return true;
}

/**
 * \internal
 * Writes a header line to the session connection.
 *
 * \param[in] session The session for which to send the header.
 * \param[in] name The name of the header which to send. This string must reside in program memory.
 * \param[in] value The header value. This string must reside in program memory.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_write_header_PP(const struct httpd_session* session, PGM_P name, PGM_P value)
{
    if(tcp_write_string_P(session->socket, name) < 1)
        return false;
    if(tcp_write_string_P(session->socket, PSTR(": ")) < 2)
        return false;
    if(tcp_write_string_P(session->socket, value) < 0)
        return false;
    if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
        return false;

    return true;
}

/**
 * \internal
 * Finishes the HTTP response header block and starts the response body.
 *
 * \param[in] session The session for which to start the response body.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_begin_content(struct httpd_session* session)
{
    if(!httpd_session_write_header_PP(session, PSTR("Connection"), PSTR("close")))
        return false;

    if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
        return false;

    return true;
}

/**
 * \internal
 * Finishes the HTTP response header block and starts the response body using the "chunked" transfer encoding.
 *
 * \param[in] session The session for which to start the response body.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_begin_content_chunked(struct httpd_session* session)
{
    if(!httpd_session_write_header_PP(session, PSTR("Connection"), PSTR("close")))
        return false;

    if(session->method != HTTPD_METHOD_HEAD)
    {
        if((session->flags & HTTPD_CLIENT_11) == HTTPD_CLIENT_11)
        {
            if(!httpd_session_write_header_PP(session, PSTR("Transfer-Encoding"), PSTR("chunked")))
                return false;

            session->flags |= HTTPD_TRANSFER_CHUNKED;
        }
    }

    if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
        return false;

    return true;
}

/**
 * \internal
 * Finishes the HTTP response and its body.
 *
 * \param[in] session The session for which to finish the response.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_end_content(struct httpd_session* session)
{
    if(session->method == HTTPD_METHOD_HEAD)
        return true;

    if(!(session->flags & HTTPD_TRANSFER_CHUNKED))
        return true;

    if(tcp_buffer_space_tx(session->socket) >= 5)
        return tcp_write_string_P(session->socket, PSTR("0\r\n\r\n")) == 5;
    else
        return true;
}

/**
 * \internal
 * Writes arbitrary data to a HTTP response body.
 *
 * \param[in] session The session for which to write the response body.
 * \param[in] content A pointer to the data which to write.
 * \param[in] content_length The number of bytes to write.
 * \returns The number of bytes written on success, \c -1 on failure.
 */
int16_t httpd_session_write_content(const struct httpd_session* session, const uint8_t* content, uint16_t content_length)
{
    if(session->method == HTTPD_METHOD_HEAD)
        return -1;

    if(content_length < 1)
        return 0;
    if(content_length > INT16_MAX)
        content_length = INT16_MAX;

    if(session->flags & HTTPD_TRANSFER_CHUNKED)
    {
        int16_t content_length_max = tcp_buffer_space_tx(session->socket);
        if(content_length_max < 9)
            return 0;
        if(content_length > content_length_max - 8)
            content_length = content_length_max - 8;

        char chunk_size_buffer[7];
        sprintf_P(chunk_size_buffer, PSTR("%04x\r\n"), content_length);
        if(tcp_write(session->socket, (uint8_t*) chunk_size_buffer, 6) < 6)
            return -1;
    }

    int16_t written = tcp_write(session->socket, content, content_length);

    if(session->flags & HTTPD_TRANSFER_CHUNKED)
    {
        if(tcp_write_string_P(session->socket, PSTR("\r\n")) < 2)
            return -1;
    }

    return written;
}

/**
 * \internal
 * Writes a string from program memory space to a HTTP response body.
 *
 * \param[in] session The session for which to write the response body.
 * \param[in] content A pointer to the string which to write. This string must reside in program memory.
 * \returns The number of bytes written on success, \c -1 on failure.
 */
int16_t httpd_session_write_content_P(const struct httpd_session* session, PGM_P content)
{
    if(session->method == HTTPD_METHOD_HEAD)
        return -1;

    int16_t queued = 0;
    int16_t len;
    char buffer[32];
    do
    {
        strncpy_P(buffer, content, sizeof(buffer));
        if(buffer[sizeof(buffer) - 1] != '\0')
            len = sizeof(buffer);
        else
            len = strlen(buffer);

        int16_t written = httpd_session_write_content(session, (uint8_t*) buffer, len);
        if(written < 0)
            return -1;

        queued += written;
        content += written;

        if(written < len)
            return queued;

    } while(len == sizeof(buffer));

    return queued;
}

/**
 * \internal
 * Retrieves the raw socket transmit buffer for direct writing outgoing data.
 *
 * Use httpd_session_get_write_buffer_size() to retrieve the size of the transmit buffer
 * and httpd_session_reserve_write_buffer() to actually mark the written data as valid.
 *
 * \param[in] session The session of which to receive the transmit buffer.
 * \returns A pointer to the transmit buffer on success, \c 0 on failure.
 */
uint8_t* httpd_session_get_write_buffer(const struct httpd_session* session)
{
    if(session->method == HTTPD_METHOD_HEAD)
        return 0;

    uint8_t* buffer = tcp_buffer_tx(session->socket);
    if(!buffer)
        return 0;

    if(session->flags & HTTPD_TRANSFER_CHUNKED)
        return buffer + 6;
    else
        return buffer;
}

/**
 * \internal
 * Retrieves the size of the session transmit buffer.
 *
 * \param[in] session The session whose transmit buffer size to retrieve.
 * \returns The size in bytes of the transmit buffer on success, \c 0 on failure.
 */
uint16_t httpd_session_get_write_buffer_size(const struct httpd_session* session)
{
    if(session->method == HTTPD_METHOD_HEAD)
        return 0;

    uint16_t size = tcp_buffer_space_tx(session->socket);
    if(size < 9)
        return 0;

    if(session->flags & HTTPD_TRANSFER_CHUNKED)
        return size - 8;
    else
        return size;
}

/**
 * \internal
 * Declares part of the session's transmit buffer as valid.
 *
 * This function must be used after writing to the buffer previously obtained
 * by calling httpd_session_get_write_buffer().
 * 
 * \param[in] session The session on whose transmit buffer to operate.
 * \param[in] len The number of bytes to mark as valid.
 * \returns The number of bytes marked as valid on success, \c -1 on failure.
 */
int16_t httpd_session_reserve_write_buffer(const struct httpd_session* session, uint16_t len)
{
    if(session->method == HTTPD_METHOD_HEAD)
        return -1;

    char* buffer = (char*) tcp_buffer_tx(session->socket);
    if(session->flags & HTTPD_TRANSFER_CHUNKED)
    {
        sprintf_P(buffer, PSTR("%04x"), len);
        buffer[4] = '\r';
        buffer[5] = '\n';

        len += 8;
    }

    int16_t reserved = tcp_reserve(session->socket, len);

    if((session->flags & HTTPD_TRANSFER_CHUNKED) && reserved > 0)
    {
        buffer[reserved + 6] = '\r';
        buffer[reserved + 7] = '\n';
    }

    return reserved;
}

/**
 * \internal
 * Retrieves the value of a session's header.
 *
 * \note The returned pointer directly points to the session's receive buffer.
 *       Modifying the string is permitted as long as the string is not extended.
 *
 * \param[in] session The session whose header value to read.
 * \param[in] name The name of the header which to search for. This string must reside in program memory.
 * \returns The string containing the header value on success, \c 0 on failure.
 */
char* httpd_session_read_header_P(const struct httpd_session* session, PGM_P name)
{
    uint16_t name_length = strlen_P(name);
    if(name_length < 1)
        return 0;

    /* peek through header lines and search for the given header name */
    char* headers = session->headers;
    while(headers[0])
    {
        if(strncasecmp_P(headers, name, name_length) == 0 && headers[name_length] == ':')
        {
            /* we found the requested header */
            headers += name_length + 1;

            /* skip whitespace */
            while(headers[0] == ' ' || headers[0] == '\011')
                ++headers;

            return headers;
        }

        /* header name does not match, skip to the next one */
        headers += strlen(headers) + 2;
    }
    
    return 0;
}

/**
 * \internal
 * Retrieves the value of a session's GET or POST argument.
 *
 * \note The returned pointer directly points to the session's receive buffer.
 *       Modifying the string is permitted as long as the string is not extended.
 *
 * \param[in] session The session whose header value to read.
 * \param[in] name The name of the argument which to search for. This string must reside in program memory.
 * \returns The string containing the header value on success, \c 0 on failure.
 */
char* httpd_session_read_argument_P(struct httpd_session* session, PGM_P name)
{
    uint16_t name_length = strlen_P(name);
    if(name_length < 1)
        return 0;

    char* arguments = session->arguments;
    char* arguments_end = session->arguments_end;
    if(!arguments)
        return 0;

    if(!arguments_end)
    {
        /* This is a POST request carrying the arguments in the body.
         */

        /* get the Content-Length header */
        char* header_content_length = httpd_session_read_header_P(session, PSTR("Content-Length"));
        if(!header_content_length)
            return 0;
        uint32_t content_length = atol(header_content_length);
        if(content_length < 1)
            return 0;

        /* check if the whole body has already arrived */
        if(content_length > tcp_buffer_used_rx(session->socket) - (arguments - (char*) tcp_buffer_rx(session->socket)))
            return 0;

        arguments_end = arguments + content_length;
        session->arguments_end = arguments_end;
    }

    if(!(session->flags & HTTPD_ARGUMENTS_DECODED))
    {
        /* split arguments */
        for(char* token = arguments; token < arguments_end; ++token)
        {
            if(*token == '&')
                token = '\0';
        }

        /* decode arguments */
        char* encoded_pos = arguments;
        char* decoded_pos = arguments;
        while(encoded_pos < arguments_end)
        {
            switch(encoded_pos[0])
            {
                case '+':
                {
                    *decoded_pos++ = ' ';
                    ++encoded_pos;
                    continue;
                }
                case '%':
                {
                    char c;

                    if(encoded_pos[1] >= '0' && encoded_pos[1] <= '9')
                        c = encoded_pos[1] - '0';
                    else if(encoded_pos[1] >= 'a' && encoded_pos[1] <= 'f')
                        c = encoded_pos[1] - 'a' + 10;
                    else if(encoded_pos[1] >= 'A' && encoded_pos[1] <= 'F')
                        c = encoded_pos[1] - 'A' + 10;
                    else
                        break;

                    c <<= 4;
                    if(encoded_pos[2] >= '0' && encoded_pos[2] <= '9')
                        c |= encoded_pos[2] - '0';
                    else if(encoded_pos[2] >= 'a' && encoded_pos[2] <= 'f')
                        c |= encoded_pos[2] - 'a' + 10;
                    else if(encoded_pos[2] >= 'A' && encoded_pos[2] <= 'F')
                        c |= encoded_pos[2] - 'A' + 10;
                    else
                        break;

                    *decoded_pos++ = c;
                    encoded_pos += 3;

                    continue;
                }
            }

            *decoded_pos++ = *encoded_pos++;
        }
        arguments_end = decoded_pos;
        session->arguments_end = arguments_end;

        session->flags |= HTTPD_ARGUMENTS_DECODED;
    }
    *arguments_end = '\0';

    /* search argument */
    while(arguments < arguments_end)
    {
        if(strncmp_P(arguments, name, name_length) == 0 && arguments[name_length] == '=')
            return arguments + name_length + 1;

        arguments += strlen(arguments) + 1;
    }

    return arguments_end;
}

/**
 * \internal
 * Closes the session and terminates the connection.
 *
 * \param[in] session The session to close.
 * \returns \c true on success, \c false on failure.
 */
bool httpd_session_close(struct httpd_session* session)
{
    tcp_disconnect(session->socket);
    return true;
}

/**
 * Retrieves a message string for the given status code.
 *
 * The returned string resides in program memory space.
 *
 * \param[in] status The status code for which to retrieve the message.
 * \returns A pointer to the message in program memory space on success, \c 0 on failure.
 */
PGM_P httpd_session_get_status_message(uint16_t status)
{
    for(uint8_t i = 0; i < sizeof(httpd_messages) / sizeof(httpd_messages[0]); ++i)
    {
        uint16_t code = pgm_read_word_near(&httpd_messages[i].code);
        if(code == status)
            return (PGM_P) pgm_read_word_near(&httpd_messages[i].message);
    }

    return 0;
}
