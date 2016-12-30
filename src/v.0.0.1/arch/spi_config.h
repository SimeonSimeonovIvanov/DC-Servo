
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef SPI_CONFIG_H
#define SPI_CONFIG_H

/**
 * \addtogroup arch
 *
 * @{
 */
/**
 * \addtogroup arch_spi
 *
 * @{
 */
/**
 * \file
 * SPI support configuration (license: GPLv2 or LGPLv2.1)
 */

/* defines for customisation of sd/mmc port access */
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega48__) || \
    defined(__AVR_ATmega88__) || \
    defined(__AVR_ATmega168__)
    #define spi_config_pin_mosi() DDRB |= (1 << DDB3)
    #define spi_config_pin_sck() DDRB |= (1 << DDB5)
    #define spi_config_pin_miso() DDRB &= ~(1 << DDB4)
#elif defined(__AVR_ATmega16__) || \
      defined(__AVR_ATmega32__)
    #define spi_config_pin_mosi() DDRB |= (1 << DDB5)
    #define spi_config_pin_sck() DDRB |= (1 << DDB7)
    #define spi_config_pin_miso() DDRB &= ~(1 << DDB6)
#elif defined(__AVR_ATmega64__) || \
      defined(__AVR_ATmega128__) || \
      defined(__AVR_ATmega169__)
    #define spi_config_pin_mosi() DDRB |= (1 << DDB2)
    #define spi_config_pin_sck() DDRB |= (1 << DDB1)
    #define spi_config_pin_miso() DDRB &= ~(1 << DDB3)
#else
    #error "no spi pin mapping available!"
#endif

/**
 * @}
 * @}
 */

#endif

