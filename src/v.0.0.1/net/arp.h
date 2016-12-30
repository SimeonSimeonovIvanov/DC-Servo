
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARP_H
#define ARP_H

#include <stdbool.h>
#include <stdint.h>

struct arp_header;

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
 * \file
 * ARP header (license: GPLv2)
 *
 * \author Roland Riegel
 */

void arp_init();
bool arp_handle_packet(const struct arp_header* packet, uint16_t packet_len);
bool arp_get_mac(const uint8_t* ip, uint8_t* mac);

/**
 * @}
 * @}
 * @}
 */

#endif

