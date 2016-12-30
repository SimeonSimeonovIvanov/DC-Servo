
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef HTTPD_MODULES_H
#define HTTPD_MODULES_H

#include <stdbool.h>
#include <stdint.h>

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
 * HTTP server modules header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * \internal
 * The reason because of which the module is executed.
 */
enum httpd_module_reason
{
    /** \internal Asks the module wether it can handle the session. */
    HTTPD_MODULE_REASON_CAN_HANDLE_REQUEST = 0,
    /** \internal Advises the module to handle the session. */
    HTTPD_MODULE_REASON_HANDLE_REQUEST,
    /** \internal Advises the module to continue handling the session (if needed). */
    HTTPD_MODULE_REASON_HANDLE_REQUEST_CONTINUE,
    /** \internal Asks the module to perform any cleanup operations neccessary. */
    HTTPD_MODULE_REASON_CLEANUP
};

struct httpd_session;

/**
 * \internal
 * The definition of a module.
 *
 * Every module consists of a single callback function.
 */
typedef bool (*httpd_module_callback)(struct httpd_session* session, enum httpd_module_reason reason);

bool httpd_module_get_status_xml_callback(struct httpd_session* session, enum httpd_module_reason reason);
bool httpd_module_do_togle_callback(struct httpd_session* session, enum httpd_module_reason reason);

bool httpd_module_dir_callback(struct httpd_session* session, enum httpd_module_reason reason);
bool httpd_module_file_callback(struct httpd_session* session, enum httpd_module_reason reason);

bool httpd_module_root_callback(struct httpd_session* session, enum httpd_module_reason reason);
bool httpd_module_auth_callback(struct httpd_session* session, enum httpd_module_reason reason);
bool httpd_module_demo_arg_callback(struct httpd_session* session, enum httpd_module_reason reason);

/**
 * @}
 * @}
 */

#endif

