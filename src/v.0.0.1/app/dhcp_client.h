
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H

#include <stdbool.h>

/**
 * \addtogroup app
 *
 * @{
 */
/**
 * \addtogroup app_dhcp_client
 *
 * @{
 */
/**
 * \file
 * DHCP client header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * The events the DHCP client may fire.
 */
enum dhcp_client_event
{
    /** The network has been successfully configured. */
    DHCP_CLIENT_EVT_LEASE_ACQUIRED = 0,
    /** The DHCP server has denied a configuration. */
    DHCP_CLIENT_EVT_LEASE_DENIED,
    /** The network configuration is about to expire, a new configuration cycle is being started. */
    DHCP_CLIENT_EVT_LEASE_EXPIRING,
    /** The network configuration has expired and could not be renewed. The network interface should be deconfigured. */
    DHCP_CLIENT_EVT_LEASE_EXPIRED,
    /** No DHCP server responded to our requests. */
    DHCP_CLIENT_EVT_TIMEOUT,
    /** An unknown error occured. */
    DHCP_CLIENT_EVT_ERROR
};

/**
 * The type of callback function used for issuing events.
 */
typedef void (*dhcp_client_callback)(enum dhcp_client_event event);

bool dhcp_client_start(dhcp_client_callback callback);
void dhcp_client_abort();

/**
 * @}
 * @}
 */

#endif

