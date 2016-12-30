
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "net.h"

/**
 * \addtogroup net Network interface
 *
 * This module implements the network interconnect.
 *
 * @{
 */
/**
 * \addtogroup net_stack Protocol stack
 *
 * This module contains the implementation of the complete network protocol stack.
 * Each submodule handles a different protocol.
 *
 * @{
 */
/**
 * \addtogroup net_stack_util Network utility functions and macros
 *
 * This module provides some helper functions and macros
 * used throughout the network stack.
 *
 * @{
 */
/**
 * \file
 * Network utility functions (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * Generic Internet checksum implementation.
 *
 * Calculates the Internet checksum over some arbitrary data block. Updating the checksum
 * is possible by providing the checksum of some previous data.
 *
 * \param[in] checksum The checksum value to start from. Should be zero for the first byte.
 * \param[in] data A pointer to the buffer containing the data to be checksummed.
 * \param[in] data_len The number of bytes to be checksummed.
 * \param[in] skip The position of a two-bytes-value assumed to be zero during checksum calculation.
 * \returns The inverted checksum value.
 */
uint16_t net_calc_checksum(uint16_t checksum, const uint8_t* data, uint16_t data_len, uint8_t skip)
{
    if(data_len < 1)
        return checksum;

    const uint8_t* data_skip = data + skip;
    const uint8_t* data_end = data + data_len - 1;

#ifndef __AVR__
    uint16_t w;
    for(; data < data_end; data += 2)
    {
        /* skip the checksum bytes within the data */
        if(data == data_skip)
            continue;

        w = ((uint16_t) data[0]) << 8 |
            ((uint16_t) data[1]);

        checksum += w;
        if(checksum < w)
            ++checksum;
    }
    if(data == data_end)
    {
        w = ((uint16_t) *data_end) << 8;
        checksum += w;
        if(checksum < w)
            ++checksum;
    }
#else
    __asm__ __volatile__ (
        "0:                                    \n\t"
        "    cp %A[data_skip], %A[data]        \n\t"
        "    cpc %B[data_skip], %B[data]       \n\t"
        "    breq 1f                           \n\t"
        "    ldd __tmp_reg__, %a[data]+1       \n\t"
        "    add %A[checksum], __tmp_reg__     \n\t"
        "    ld __tmp_reg__, %a[data]          \n\t"
        "    adc %B[checksum], __tmp_reg__     \n\t"
        "    adc %A[checksum], __zero_reg__    \n\t"
        "    adc %B[checksum], __zero_reg__    \n\t"
        "1:                                    \n\t"
        "    adiw %A[data], 2                  \n\t"
        "    cp %A[data], %A[data_end]         \n\t"
        "    cpc %B[data], %B[data_end]        \n\t"
        "    brlo 0b                           \n\t"
        "    brne 2f                           \n\t"
        "    ld __tmp_reg__, %a[data]          \n\t"
        "    add %B[checksum], __tmp_reg__     \n\t"
        "    adc %A[checksum], __zero_reg__    \n\t"
        "    adc %B[checksum], __zero_reg__    \n\t"
        "2:                                    \n\t"
        : [checksum] "+r" (checksum)
        : [data] "b" (data), [data_skip] "r" (data_skip), [data_end] "r" (data_end)
    );
#endif

    return checksum;
}

/**
 * Converts a 16-bit integer to network byte order.
 *
 * Use this function on variable values instead of the
 * macro HTON16(). This saves code size.
 *
 * \param[in] h A 16-bit integer in host byte order.
 * \returns The given 16-bit integer converted to network byte order.
 */
uint16_t hton16(uint16_t h)
{
#ifndef __AVR__
    return HTON16(h);
#else
    __asm__ __volatile__ (
        "mov __tmp_reg__, %A0   \n\t"
        "mov %A0, %B0           \n\t"
        "mov %B0, __tmp_reg__   \n\t"
        : "+r" (h)
    );

    return h;
#endif
}

/**
 * Converts a 32-bit integer to network byte order.
 *
 * Use this function on variable values instead of the
 * macro HTON32(). This saves code size.
 *
 * \param[in] h A 32-bit integer in host byte order.
 * \returns The given 32-bit integer converted to network byte order.
 */
uint32_t hton32(uint32_t h)
{
#ifndef __AVR__
    return HTON32(h);
#else
    __asm__ __volatile__ (
        "mov __tmp_reg__, %A0   \n\t"
        "mov %A0, %D0           \n\t"
        "mov %D0, __tmp_reg__   \n\t"
        "mov __tmp_reg__, %B0   \n\t"
        "mov %B0, %C0           \n\t"
        "mov %C0, __tmp_reg__   \n\t"
        : "+r" (h)
    );

    return h;
#endif
}

/**
 * @}
 * @}
 * @}
 */

