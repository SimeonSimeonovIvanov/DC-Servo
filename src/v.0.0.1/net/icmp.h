
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ICMP_H
#define ICMP_H

#include <stdbool.h>
#include <stdint.h>

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
 * \addtogroup net_stack_icmp
 *
 * @{
 */
/**
 * \file
 * ICMP header (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct icmp_header;

void icmp_init();
bool icmp_handle_packet(const uint8_t* ip, const struct icmp_header* packet, uint16_t packet_len);

/**
 * @}
 * @}
 * @}
 */

#endif

