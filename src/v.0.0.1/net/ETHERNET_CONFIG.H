
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ETHERNET_CONFIG_H
#define ETHERNET_CONFIG_H

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
 * \addtogroup net_stack_ethernet
 *
 * @{
 */
/**
 * \addtogroup net_stack_ethernet_config Ethernet configuration
 *
 * @{
 */
/**
 * \file
 * Ethernet configuration (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * Maximum ethernet packet payload.
 *
 * This cannot be larger than \c 1500, and should only be smaller
 * when RAM restrictions require this.
 */
#define ETHERNET_MAX_PACKET_SIZE 1500

/**
 * @}
 * @}
 * @}
 * @}
 */

#endif

