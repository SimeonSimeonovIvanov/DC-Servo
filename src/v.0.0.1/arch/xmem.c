
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>

/**
 * \addtogroup arch
 *
 * @{
 */
/**
 * \addtogroup arch_xmem External RAM support
 *
 * Extends the amount of available RAM.
 *
 * \note This is highly hardware and setup dependent. The linker must know
 *       the memory layout for this to work as expected.
 * \note At the moment, the hardware interface for external RAM is
 *       configured for 64 kBytes of external SRAM without wait-states.
 *
 * @{
 */
/**
 * \file
 * External RAM initialization (license: GPLv2)
 *
 * \author Roland Riegel
 */

/* make sure this is executed before the .data section is initialized */
void xmem_init() __attribute__((naked)) __attribute__((section(".init3")));

/**
 * Initializes the hardware interface for external RAM usage.
 *
 * \note This function is automatically called before application startup. Do
 *       not call it explicitely.
 */
void xmem_init()
{
	/* single ram sector */
	XMCRA &= ~((1 << SRL2) | (1 << SRL1) | (1 << SRL0));
	/* no wait states */
	MCUCR &= ~(1 << SRW10);
	XMCRA &= ~(1 << SRW11);
	XMCRB = (0 << XMBK) | (0 << XMM2) | (0 << XMM1) | (0 << XMM0);
	/* enable xmem */
	MCUCR |= (1 << SRE);
}

/**
 * @}
 * @}
 */
