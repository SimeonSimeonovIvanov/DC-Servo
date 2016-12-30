
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NET_H
#define NET_H

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
 * \addtogroup net_stack_util
 *
 * @{
 */
/**
 * \file
 * Network utility header (license: GPLv2)
 *
 * \author Roland Riegel
 */

/* byte order handling */

/**
 * \def HTON16(val)
 *
 * Converts a 16-bit integer to network byte order.
 *
 * Use this macro for compile time constants only. For variable values
 * use the function hton16() instead. This saves code size.
 *
 * \param[in] val A 16-bit integer in host byte order.
 * \returns The given 16-bit integer converted to network byte order.
 */
/**
 * \def HTON32(val)
 *
 * Converts a 32-bit integer to network byte order.
 *
 * Use this macro for compile time constants only. For variable values
 * use the function hton32() instead. This saves code size.
 *
 * \param[in] val A 32-bit integer in host byte order.
 * \returns The given 32-bit integer converted to network byte order.
 */

#if BIG_ENDIAN
#define HTON16(val) (val)
#define HTON32(val) (val)
#else
#define HTON16(val) ((((uint16_t) (val)) << 8) | \
                     (((uint16_t) (val)) >> 8)   \
                    )
#define HTON32(val) (((((uint32_t) (val)) & 0x000000ff) << 24) | \
                     ((((uint32_t) (val)) & 0x0000ff00) <<  8) | \
                     ((((uint32_t) (val)) & 0x00ff0000) >>  8) | \
                     ((((uint32_t) (val)) & 0xff000000) >> 24)   \
                    )
#endif


/**
 * Converts a 16-bit integer to host byte order.
 *
 * Use this macro for compile time constants only. For variable values
 * use the function ntoh16() instead. This saves code size.
 *
 * \param[in] val A 16-bit integer in network byte order.
 * \returns The given 16-bit integer converted to host byte order.
 */
#define NTOH16(val) HTON16(val)

/**
 * Converts a 32-bit integer to host byte order.
 *
 * Use this macro for compile time constants only. For variable values
 * use the function ntoh32() instead. This saves code size.
 *
 * \param[in] val A 32-bit integer in network byte order.
 * \returns The given 32-bit integer converted to host byte order.
 */
#define NTOH32(val) HTON32(val)

/* header sizes */

/** The size of an ethernet header. */
#define NET_HEADER_SIZE_ETHERNET 14
/** The size of an IP header. */
#define NET_HEADER_SIZE_IP 20
/** The size of a TCP header. */
#define NET_HEADER_SIZE_TCP 20
/** The size of a UDP header. */
#define NET_HEADER_SIZE_UDP 8

/* checksum calculation */

uint16_t net_calc_checksum(uint16_t checksum, const uint8_t* data, uint16_t data_len, uint8_t skip);
uint16_t hton16(uint16_t h);
uint32_t hton32(uint32_t h);

/**
 * Converts a 16-bit integer to host byte order.
 *
 * Use this function on variable values instead of the
 * macro NTOH16(). This saves code size.
 *
 * \param[in] n A 16-bit integer in network byte order.
 * \returns The given 16-bit integer converted to host byte order.
 */
#define ntoh16(n) hton16(n)

/**
 * Converts a 32-bit integer to host byte order.
 *
 * Use this function on variable values instead of the
 * macro NTOH32(). This saves code size.
 *
 * \param[in] n A 32-bit integer in network byte order.
 * \returns The given 32-bit integer converted to host byte order.
 */
#define ntoh32(n) hton32(n)

/**
 * @}
 * @}
 * @}
 */

#endif

