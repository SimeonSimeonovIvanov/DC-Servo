
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CLOCK_SYNC_CONFIG_H
#define CLOCK_SYNC_CONFIG_H

/**
 * \addtogroup app
 *
 * @{
 */
/**
 * \addtogroup app_clock_sync
 *
 * @{
 */
/**
 * \addtogroup app_clock_sync_config Clock synchronization configuration
 *
 * \note In the official firmware my own server is contacted. This is for
 *       illustration purposes only. The service is not guaranteed and should
 *       not be used more often than every ten minutes!
 *
 * @{
 */
/**
 * \file
 * TIME client configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/** Interval in seconds between synchronization attempts. */
#define CLOCK_SYNC_INTERVAL (20 * 60)

/** Timeout in seconds of synchronization attempts. */
#define CLOCK_SYNC_TIMEOUT 10

/** IP address (first byte) of the remote host which to synchronize from. */
#define CLOCK_SYNC_IP_0 17
/** IP address (second byte) of the remote host which to synchronize from. */
#define CLOCK_SYNC_IP_1 254
/** IP address (third byte) of the remote host which to synchronize from. */
#define CLOCK_SYNC_IP_2 0
/** IP address (fourth byte) of the remote host which to synchronize from. */
#define CLOCK_SYNC_IP_3 31

/**
 * @}
 * @}
 * @}
 */

#endif

