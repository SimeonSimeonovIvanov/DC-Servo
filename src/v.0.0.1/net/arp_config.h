
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARP_CONFIG_H
#define ARP_CONFIG_H

/**
 * \addtogroup net
 *
 * @{
 */
/**
 * \addtogroup net_stack
 *
 * @{
 */
/**
 * \addtogroup net_stack_arp
 *
 * @{
 */
/**
 * \addtogroup net_stack_arp_config ARP configuration
 *
 * @{
 */
/**
 * \file
 * ARP configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * The maximum number of ARP table entries.
 */
#define ARP_TABLE_SIZE 4

/**
 * The expiration time of an ARP table entry in units of five seconds.
 */
#define ARP_TIMEOUT (5 * 60 / 5) /* 5 minutes */

/**
 * @}
 * @}
 * @}
 * @}
 */

#endif

