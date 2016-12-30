
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef HTTPD_H
#define HTTPD_H

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
 * HTTP server main header (license: GPLv2)
 *
 * \author Roland Riegel
 */

bool httpd_init(uint16_t port);

/**
 * @}
 * @}
 */

#endif

