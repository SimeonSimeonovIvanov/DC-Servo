/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <ctype.h>
#include <stdlib.h>
#include <avr/wdt.h>

#include "httpd_config.h"
#include "httpd_modules.h"
#include "httpd_session.h"

#include "../sd/fat16.h"
#include "../sd/sd.h"
#include "../main.h"

#include <stdio.h>
#include <string.h>
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
 * HTTP server module implementations (license: GPLv2)
 *
 * Current modules provide support for:
 * - HTTP Basic authorization.
 * - Custom start website.
 * - File download from the memory card.
 * - Directory listings from the memory card.
 *
 * \author Roland Riegel
 */

struct httpd_module_mime_mapping
{
	const char* file_ending;
	const char* mime;
};

extern volatile uint8_t inPort[];
extern volatile uint8_t outPort[];
extern uint16_t uiRegInputBuf[];
extern uint16_t uiRegHolding[];
extern uint8_t ucRegCoilsBuf[];
extern uint16_t arrDAC[];
extern int32_t nEncoderPosition;

extern void ftoa(float f, char *buffer);

static const char PROGMEM httpd_module_file_ending_css[] = ".css";
static const char PROGMEM httpd_module_file_ending_gif[] = ".gif";
static const char PROGMEM httpd_module_file_ending_html[] = ".html";
static const char PROGMEM httpd_module_file_ending_htm[] = ".htm";
static const char PROGMEM httpd_module_file_ending_jpeg[] = ".jpeg";
static const char PROGMEM httpd_module_file_ending_jpg[] = ".jpg";
static const char PROGMEM httpd_module_file_ending_js[] = ".js";
static const char PROGMEM httpd_module_file_ending_png[] = ".png";
static const char PROGMEM httpd_module_file_ending_txt[] = ".txt";
static const char PROGMEM httpd_module_file_ending_xml[] = ".xml";
static const char PROGMEM httpd_module_file_ending_zip[] = ".zip";
static const char PROGMEM httpd_module_file_ending_cgi[] = ".cgi";

static const char PROGMEM httpd_module_mime_css[] = "text/css";
static const char PROGMEM httpd_module_mime_gif[] = "image/gif";
static const char PROGMEM httpd_module_mime_html[] = "text/html";
static const char PROGMEM httpd_module_mime_jpeg[] = "image/jpeg";
static const char PROGMEM httpd_module_mime_js[] = "text/javascript";
static const char PROGMEM httpd_module_mime_png[] = "image/png";
static const char PROGMEM httpd_module_mime_txt[] = "text/plain";
static const char PROGMEM httpd_module_mime_xml[] = "text/xml";
static const char PROGMEM httpd_module_mime_zip[] = "application/zip";
static const char PROGMEM httpd_module_mime_octet[] = "application/octet-stream";
static const char PROGMEM httpd_module_mime_cgi[] = "application/cgi";

struct httpd_module_mime_mapping httpd_module_mime_mappings[] PROGMEM =
{
	{ httpd_module_file_ending_css,  httpd_module_mime_css	},
	{ httpd_module_file_ending_gif,  httpd_module_mime_gif	},
	{ httpd_module_file_ending_html, httpd_module_mime_html	},
	{ httpd_module_file_ending_htm,  httpd_module_mime_html	},
	{ httpd_module_file_ending_jpeg, httpd_module_mime_jpeg	},
	{ httpd_module_file_ending_jpg,  httpd_module_mime_jpeg	},
	{ httpd_module_file_ending_js,   httpd_module_mime_js	},
	{ httpd_module_file_ending_png,  httpd_module_mime_png	},
	{ httpd_module_file_ending_txt,  httpd_module_mime_txt	},
	{ httpd_module_file_ending_xml,  httpd_module_mime_xml	},
	{ httpd_module_file_ending_zip,  httpd_module_mime_zip	},
	{ httpd_module_file_ending_cgi,  httpd_module_mime_cgi	},
};

static PGM_P httpd_module_get_mime_type(const char* uri);
static void httpd_module_base64_decode(char* str);

char data_buffer[1000];

bool httpd_module_get_status_xml_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	static unsigned long fileLen = 0;

	switch(reason) {
	case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST: {
		int i, urlLen = strlen(session->uri) - 1;
		char length_string[12], buffer[100], sz_float[10];;

		while( urlLen >= 0 && session->uri[urlLen] && '/' != session->uri[urlLen] ) {
			--urlLen;
		}

		if( strncmp_P(session->uri + urlLen, PSTR("/status.xml"), 11) ) {
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		sprintf(data_buffer, "<response>\n");

		for(i = 0; i < 10; i++) {
			sprintf(buffer, "<DI%d>%d</DI%d>\n", i, inPort[i], i);
			strcat(data_buffer, buffer);
		}

		sprintf(buffer, "<FB>%d</FB>\n", inPort[10]);
		strcat(data_buffer, buffer);

		for(i = 0; i < 3; i++) {
			sprintf(buffer, "<FB%d>%d</FB%d>\n", i, inPort[11 + i], i);
			strcat(data_buffer, buffer);
		}

		sprintf( buffer, "<ENC_Step>%d</ENC_Step>\n", (int)nEncoderPosition );
		strcat(data_buffer, buffer);

		for(i = 0; i < 12; i++) {
			sprintf(buffer, "<DO%d>%d</DO%d>\n", i, outPort[i], i);
			strcat(data_buffer, buffer);
		}

		for(i = 0; i < 7; i++) {
			sprintf(buffer, "<AI%d>%d</AI%d>\n", i, uiRegInputBuf[i], i);
			strcat(data_buffer, buffer);
		}

		for(i = 0; i < 2; i++) {
			sprintf(buffer, "<AO%d>%u</AO%d>\n", i, arrDAC[i], i);
			strcat(data_buffer, buffer);
		}
		
		if( 830 >= uiRegInputBuf[0] ) {
			ftoa(36.5/(815.0 - 90.0) * ((double)uiRegInputBuf[0]-90), sz_float);
		} else
		if( 2160 >= uiRegInputBuf[0] ) {
			ftoa(100.0/(2140.0 - 90.0) * ((double)uiRegInputBuf[0]-90), sz_float);
		} else {
			ftoa(200.0/(4002.0-90.0) * ((double)uiRegInputBuf[0]-90), sz_float);
		}

		sprintf( buffer, "<T0>%s</T0>", sz_float );
		strcat(data_buffer, buffer);

		strcat(data_buffer, "</response>\n");
		//////////////////////////////////////////////////////////////////////////
		fileLen = strlen(data_buffer);
		sprintf_P(length_string, PSTR("%lu"), fileLen);

		httpd_session_write_status(session, 200);
		httpd_session_write_header_P(session, PSTR("Content-Length"), length_string);
		httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/xml"));
		httpd_session_begin_content(session);
	}
	 return true;

	case HTTPD_MODULE_REASON_HANDLE_REQUEST:
	case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE: {
		uint16_t space = httpd_session_get_write_buffer_size(session);
		uint8_t* lpHttpdWriteBuffer = httpd_session_get_write_buffer(session);

		wdt_reset();
		do {
			if(!lpHttpdWriteBuffer || space < 1) {
				break;
			}

			strncpy( (char*)lpHttpdWriteBuffer, data_buffer, fileLen );
			if( (1 + fileLen) != httpd_session_reserve_write_buffer(session, fileLen) ) {
				break;
			}

			return true;
		} while(0);

		httpd_session_end_content(session);
		httpd_session_close(session);
		return true;
	}
	 break;

	case HTTPD_MODULE_REASON_CLEANUP:
		fileLen = 0;
	 return true;
	}

	return false;
}

bool httpd_module_do_togle_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	static unsigned long fileLen = 0;

	switch(reason) {
	case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST: {
		int i, len = strlen(session->uri) - 1;
		char *lpIndex;

		while( len >= 0 && session->uri[len] && '/' != session->uri[len] ) {
			--len;
		}

		if( strncmp_P(session->uri + len, PSTR("/do_togle"), 9) ) {
			return false;
		}

		if( NULL == (lpIndex = httpd_session_read_argument_P(session, PSTR("index"))) ) {
			return false;
		}

		i = atoi(lpIndex);

		if( i >= 0 && i < 12 ) {
			char length_string[12];
			char buffer[100];

			ucRegCoilsBuf[i/8] ^= 1<<(i - 8*(i/8));
			//////////////////////////////////////////////////////
			sprintf(data_buffer, "<response>\n");
			
			sprintf(
				buffer,
				"<DO%d>%d</DO%d>\n",
				i,
				( ucRegCoilsBuf[i/8] & 1<<(i - 8*(i/8)) ) ? 1:0,
				i
			);
			strcat(data_buffer, buffer);
			
			strcat(data_buffer, "</response>\n");
			//////////////////////////////////////////////////////
			fileLen = strlen(data_buffer);
			sprintf_P(length_string, PSTR("%lu"), fileLen);

			httpd_session_write_status(session, 200);
			httpd_session_write_header_P(session, PSTR("Content-Length"), length_string);
			httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/xml"));
			httpd_session_begin_content(session);

			return true;
		}
	}
	 break;

	case HTTPD_MODULE_REASON_HANDLE_REQUEST:
	case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE: {
		uint16_t space = httpd_session_get_write_buffer_size(session);
		uint8_t* lpHttpdWriteBuffer = httpd_session_get_write_buffer(session);

		wdt_reset();
		do {
			if(!lpHttpdWriteBuffer || space < 1) {
				break;
			}

			strncpy( (char*)lpHttpdWriteBuffer, data_buffer, fileLen );
			if( (1 + fileLen) != httpd_session_reserve_write_buffer(session, fileLen) ) {
				break;
			}
		} while(0);


		httpd_session_end_content(session);
		httpd_session_close(session);
	}
	 return true;

	case HTTPD_MODULE_REASON_CLEANUP:
		fileLen = 0;
	 return true;

	}

	return false;
}

/**
 * \internal
 * A module for providing directory listings.
 *
 * If the request URI corresponds to a directory on the connected memory card,
 * it generates a listing with each entry corresponding to a file or a subdirectory
 * of that directory.
 *
 * For directory listing requests without final slash (e.g. "/directory"
 * instead of "/directory/") the HTTP client gets forwarded to the URL including
 * the slash.
 */
bool httpd_module_dir_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	struct fat16_dir_struct* dd = (struct fat16_dir_struct*) session->user;
	struct fat16_dir_entry_struct dir_entry;

	switch(reason) {
		case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST:{
			if(strncmp_P(session->uri, PSTR("/sd/"), 4) != 0)
				return false;

			if(!fat16_get_dir_entry_of_path(sd_get_fs(), session->uri + 3, &dir_entry))
				return false;

			if(!(dir_entry.attributes & FAT16_ATTRIB_DIR))
				return false;

			uint8_t uri_len = strlen(session->uri);
			if(session->uri[uri_len - 1] != '/')
			{
				/* this is dirty, but works for now */
				session->uri[uri_len] = '/';
				session->uri[uri_len + 1] = '\0';

				httpd_session_write_status(session, 302);
				httpd_session_write_header_P(session, PSTR("Location"), session->uri);
				httpd_session_begin_content(session);
				httpd_session_end_content(session);
				httpd_session_close(session);
				return true;
			}

			session->user = (uint16_t) fat16_open_dir(sd_get_fs(), &dir_entry);
			dd = (struct fat16_dir_struct*) session->user;

			if(!dd) {
				httpd_session_write_error(session, 404);
				httpd_session_close(session);
				return true;
			}

			httpd_session_write_status(session, 200);
			httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/html"));
			httpd_session_begin_content(session);
			httpd_session_write_content_P(session, PSTR("<html><h1>Directory listing of "));
			httpd_session_write_content(session, (uint8_t*) session->uri, uri_len);
			httpd_session_write_content_P(session, PSTR("</h1><body><pre><table>"));
			return true;
		}
		case HTTPD_MODULE_REASON_HANDLE_REQUEST:
		case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE:
		{
			while(1) {
				char* buffer = (char*) httpd_session_get_write_buffer(session);
				uint16_t space = httpd_session_get_write_buffer_size(session);
				if(space < 2 * sizeof(dir_entry.long_name) + 42 + 10)
					return true;
				if(!buffer)
					break;

				if(!fat16_read_dir(dd, &dir_entry))
					break;

				uint8_t written = sprintf_P(buffer, PSTR("<tr><td>"));
				PGM_P format;
				if(dir_entry.attributes & FAT16_ATTRIB_DIR)
					format = PSTR("<a href=\"%s/\">%s/</a>");
				else
					format = PSTR("<a href=\"%s\">%s</a>");
				written += sprintf_P(buffer + written, format, dir_entry.long_name, dir_entry.long_name);
				written += sprintf_P(buffer + written, PSTR("</td><td>%lu</td></tr>"), dir_entry.file_size);

				if(httpd_session_reserve_write_buffer(session, written) != written)
					break;
			}

			httpd_session_write_content_P(session, PSTR("</table></pre></body></html>"));
			httpd_session_end_content(session);
			httpd_session_close(session);
			return true;
		}
		case HTTPD_MODULE_REASON_CLEANUP:
		{
			fat16_close_dir(dd);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

/**
 * \internal
 * A module for providing file access.
 *
 * If the request URI corresponds to a file on the connected memory card, it responds
 * with the contents of that file. That is, it actually enables the client to download
 * files from the card.
 *
 * The size of the file is included in a Content-Length header, informing the HTTP
 * client of the size of the transfer. This also enables it to display a progress bar
 * when downloading the file.
 *
 * The file extension is used to look up the mime-type of the file, which is then sent
 * in a Content-Type header and thus tells the HTTP client how to handle the file.
 */
bool httpd_module_file_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	struct fat16_file_struct *fd = (struct fat16_file_struct*)session->user;

	switch(reason) {
	case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST: {
		struct fat16_dir_entry_struct dir_entry;
		char length_string[12];

		if(strncmp(session->uri, "/sd/", 4)) {
			return false;
		}

		if(fat16_get_dir_entry_of_path(sd_get_fs(), session->uri + 3, &dir_entry)) {
			session->user = (uint16_t)fat16_open_file(sd_get_fs(), &dir_entry);
		} else {
			session->user = 0;
		}

		fd = (struct fat16_file_struct*) session->user;

		if(!fd) {
			httpd_session_write_error(session, 404);
			httpd_session_close(session);
			return false;
		}

		sprintf_P(length_string, PSTR("%lu"), dir_entry.file_size);

		httpd_session_write_status(session, 200);
		httpd_session_write_header_P(session, PSTR("Content-Length"), length_string);
		httpd_session_write_header_PP(session, PSTR("Content-Type"), httpd_module_get_mime_type(session->uri));
		httpd_session_begin_content(session);
	}
	 return true;

	case HTTPD_MODULE_REASON_HANDLE_REQUEST:
	case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE: {
		uint16_t space = httpd_session_get_write_buffer_size(session);
		uint8_t* buffer = httpd_session_get_write_buffer(session);

		wdt_reset();
		do {
			if(!buffer || space < 1) {
				break;
			}

			int16_t bytes_read = fat16_read_file(fd, buffer, space);
			if(bytes_read <= 0) {
				break;
			}

			if(bytes_read != httpd_session_reserve_write_buffer(session, bytes_read)) {
				break;
				}

			if(bytes_read < space) {
				break;
			}

			return true;
		} while(0);

		httpd_session_end_content(session);
		httpd_session_close(session);
		return true;
	}
	 break;

	case HTTPD_MODULE_REASON_CLEANUP: {
		fat16_close_file(fd);
		return true;
	}
	 break;
	}

	return false;
}

/**
 * \internal
 * A module for providing a built-in start website.
 *
 * The module only declares itself responsible for the session if the request URI
 * equals the root path "/". In this case, a welcome web page is sent to the client.
 *
 * \note It is not checked wether the HTTP response completely fits into the
 *	   transmit buffer of the connection. This would be neccessary when sending
 *	   arbitrary content.
 */
bool httpd_module_root_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	switch(reason)
	{
		case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST:
		{
			return strcmp_P(session->uri, PSTR("/")) == 0;
		}
		case HTTPD_MODULE_REASON_HANDLE_REQUEST:
		case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE:
		{
			char* val = httpd_session_read_argument_P(session, PSTR("val"));
			if(!val)
				return false;

			httpd_session_write_status(session, 200);
			httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/html"));
			httpd_session_begin_content_chunked(session);

			httpd_session_write_content_P(session, PSTR(
				"<html><body>"
				"<h1>Welcome to mega-eth!</h1>"
				"<p>Congratulations! You have your personal version of mega-eth up and running!</p>"
				"<p>What you can do now is:</p>"
				"<ul>"
					"<li>Browse the <a href=\"sd/\">content of the connected memory card</a>.</li>"
					"<li>Look at a <a href=\"demo_get\">demo page which shows how to evaluate GET parameters</a>.</li>"
					"<li>Do <a href=\"demo_post\">the same for POST parameters</a>.</li>"
				"</ul>"
				"</body></html>"
			));

			httpd_session_end_content(session);
			httpd_session_close(session);
		}
		case HTTPD_MODULE_REASON_CLEANUP:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

/**
 * \internal
 * A module for handling authorization with the configured user and password.
 *
 * The authorization is checked when the module is asked if it can handle the request.
 * If the client provided an incorrect user name or password, the module takes over
 * responsibility and sends an error message back. In the successful case, the module
 * rejects responsibility and the session will be handled by another module which
 * will generate the protected content.
 */
bool httpd_module_auth_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	switch(reason)
	{
		case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST:
		{
			char* authorization = httpd_session_read_header_P(session, PSTR("Authorization"));
			if(!authorization)
				return true;
			if(strncmp_P(authorization, PSTR("Basic "), 6) != 0)
				return true;

			httpd_module_base64_decode(&authorization[6]);
			if(strcmp_P(&authorization[6], PSTR(HTTPD_AUTH_USER ":" HTTPD_AUTH_PASSWORD)) != 0)
				return true;

			return false;
		}
		case HTTPD_MODULE_REASON_HANDLE_REQUEST:
		{
			httpd_session_write_status(session, 401);
			httpd_session_write_header_PP(session, PSTR("WWW-Authenticate"), PSTR("Basic realm=\"" HTTPD_AUTH_REALM "\""));
			httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/html"));
			httpd_session_begin_content_chunked(session);
			httpd_session_write_content_P(session, PSTR("401 - Unauthorized"));
			httpd_session_end_content(session);
			httpd_session_close(session);
			return true;
		}
		case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE:
		{
			return false;
		}
		case HTTPD_MODULE_REASON_CLEANUP:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

/**
 * \internal
 * A module demonstrating usage of GET or POST arguments.
 */
bool httpd_module_demo_arg_callback(struct httpd_session* session, enum httpd_module_reason reason)
{
	switch(reason) {
	case HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST: {
		if(strcmp_P(session->uri, PSTR("/demo_get")) == 0) {
			session->user = 0;
		} else
		if(strcmp_P(session->uri, PSTR("/demo_post")) == 0) {
			session->user = 1;
		} else
		if(strcmp_P(session->uri, PSTR("/pot")) == 0) {
			session->user = 2;
		} else {
			return false;
		}
	 }
	  return true;

	case HTTPD_MODULE_REASON_HANDLE_REQUEST:
	case HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE: {
		PGM_P method = PSTR("GET");
		char* val = httpd_session_read_argument_P(session, PSTR("val"));

		if(1 == session->user) {
			method = PSTR("POST");
		} else

		if(!val && session->user < 3) {
			return false;
		}

		if(session->user < 3) {
			httpd_session_write_status(session, 200);
			httpd_session_write_header_PP(session, PSTR("Content-Type"), PSTR("text/html"));
			httpd_session_begin_content_chunked(session);

			httpd_session_write_content_P(session, PSTR("<html><body><h1>mega-eth "));
			httpd_session_write_content_P(session, method);
			httpd_session_write_content_P(session, PSTR(" demo</h1>"));

			httpd_session_write_content_P(session, PSTR("<p><i>"));
			httpd_session_write_content(session, (uint8_t*) val, strlen(val));
			httpd_session_write_content_P(session, PSTR("</i></p>"));

			httpd_session_write_content_P(session, PSTR("<p><form action=\""));
			httpd_session_write_content(session, (uint8_t*) session->uri, strlen(session->uri));
			httpd_session_write_content_P(session, PSTR("\" method=\""));
			httpd_session_write_content_P(session, method);

			httpd_session_write_content_P(session, PSTR("\"><textarea name=\"val\" cols=\"50\" rows=\"10\">"));
			httpd_session_write_content(session, (uint8_t*) val, strlen(val));
			httpd_session_write_content_P(session, PSTR("</textarea><br><input type=\"submit\" value=\""));
			httpd_session_write_content_P(session, method);
			httpd_session_write_content_P(session, PSTR("\"></form></p>"));

			httpd_session_write_content_P(session, PSTR("</body></html>"));

			httpd_session_end_content(session);
			httpd_session_close(session);
		} else {
		}
	}
	 return true;

	case HTTPD_MODULE_REASON_CLEANUP: return true;
	
	default: return false;
	}
}

/**
 * Determines the mime-type of a HTTP transfer by the file extension.
 *
 * Searches a list for a matching file extension and returns the
 * corresponding mime-type.
 *
 * \param[in] uri The filename or URI for which to determine the mime-type.
 * \returns A pointer to the mime-type string in program memory space.
 */
PGM_P httpd_module_get_mime_type(const char* uri)
{
	uint8_t i;
	char lpBuffer[255];

	for(i = 0; i < sizeof(httpd_module_mime_mappings) / sizeof(*httpd_module_mime_mappings); ++i) {
		PGM_P ending = (PGM_P)pgm_read_word_near(&httpd_module_mime_mappings[i].file_ending);
		uint8_t ending_length = strlen_P(ending);
		uint16_t uri_length = strlen(uri);

		if(uri_length < ending_length) {
			continue;
		}

		strcpy(lpBuffer, uri + uri_length - ending_length);

		for(int j = 0; lpBuffer[j]; j++) {
			lpBuffer[j] = tolower(lpBuffer[j]);
		}

		if(strcmp_P(lpBuffer, ending) != 0) {
			continue;
		}

		return (PGM_P)pgm_read_word_near(&httpd_module_mime_mappings[i].mime);
	}

	return httpd_module_mime_octet;
}

/**
 * Decodes a Base64-encoded string.
 *
 * \param[in,out] str The encoded string which gets decoded.
 */
void httpd_module_base64_decode(char* str)
{
	char stream[4];
	char* out = str;
	
	while(strlen(str) >= 4) {
		for(uint8_t i = 0; i < 4; ++i) {
			if(*str >= 'A' && *str <= 'Z') {
				stream[i] = *str - 'A';
			} else
			if(*str >= 'a' && *str <= 'z') {
				stream[i] = *str - 'a' + 26;
			} else
			if(*str >= '0' && *str <= '9') {
				stream[i] = *str - '0' + 52;
			} else
			if(*str == '+') {
				stream[i] = 62;
			}
			else
			if(*str == '/') {
				stream[i] = 63;
			} else
			if(*str == '=') {
				stream[i] = 0;
			}
			++str;
		}

		*out++ = stream[0] << 2 | stream[1] >> 4;
		*out++ = stream[1] << 4 | stream[2] >> 2;
		*out++ = stream[2] << 6 | stream[3] >> 0;
	}

	*out = '\0';
}
