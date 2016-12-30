
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SPI_H
#define SPI_H

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
 * SPI header (license: GPLv2)
 *
 * \author Roland Riegel
 */

void spi_init();

void spi_send_byte(uint8_t b);
uint8_t spi_rec_byte();

void spi_send_data(const uint8_t* data, uint16_t data_len);
void spi_rec_data(uint8_t* buffer, uint16_t buffer_len);

void spi_low_frequency();
void spi_high_frequency();

/**
 * @}
 * @}
 */

#endif

