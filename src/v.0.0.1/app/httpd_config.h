
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef HTTPD_CONFIG_H
#define HTTPD_CONFIG_H

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
 * \addtogroup app_httpd_config HTTP server configuration
 *
 * @{
 */
/**
 * \file
 * HTTP server configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/** The name of the server as it appears in HTTP response headers. */
#define HTTPD_NAME "mega-eth"

/** The maximum number of HTTP requests currently in processing. */
#define HTTPD_MAX_SESSIONS 30

/** The name of the section which needs authorization. */
#define HTTPD_AUTH_REALM "mega-eth"

/** The user name needed for a successful authorization. */
#define HTTPD_AUTH_USER "admin"

/** The password needed for a successful authorization. */
#define HTTPD_AUTH_PASSWORD "password"

/**
 * @}
 * @}
 * @}
 */

#endif

