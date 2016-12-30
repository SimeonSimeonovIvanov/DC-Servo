
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/**
 * \addtogroup arch
 *
 * @{
 */
/**
 * \addtogroup arch_uart
 *
 * @{
 */
/**
 * \file
 * UART header (license: GPLv2)
 *
 * \author Roland Riegel
 */

void uart_init();
void uart_connect_stdio();
void uart_putc(uint8_t c);
void uart_putc_hex(uint8_t b);
uint8_t uart_getc();
uint8_t uart_getc_try();

/**
 * @}
 * @}
 */

#endif

