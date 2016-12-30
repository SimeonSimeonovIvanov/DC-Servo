
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>

#include "spi.h"
#include "spi_config.h"

/**
 * \addtogroup arch Architecture and hardware specific services
 *
 * Abstract layer for access to architecture and hardware specific functions.
 *
 * @{
 */
/**
 * \addtogroup arch_spi Hardware SPI
 *
 * @{
 */
/**
 * \file
 * SPI implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * Initializes the SPI interface.
 *
 * By default, the highest SPI frequency available is used.
 */
void spi_init()
{
	if(!(SPCR & (1 << SPE))) {
		/* configure pins */
		spi_config_pin_mosi();
		spi_config_pin_sck();
		spi_config_pin_miso();
		
		/* configure SPI */
		spi_high_frequency();
	}
}

/**
 * Sends a byte over the SPI bus.
 *
 * \param[in] b The byte to send.
 */
void spi_send_byte(uint8_t b)
{
    SPDR = b;
    /* wait for byte to be shifted out */
    while(!(SPSR & (1 << SPIF)));
}

/**
 * Receives a byte from the SPI bus.
 *
 * \returns The received byte.
 */
uint8_t spi_rec_byte()
{
    /* send dummy data for receiving some */
    SPDR = 0xff;
    while(!(SPSR & (1 << SPIF)));

    return SPDR;
}

/**
 * Sends data contained in a buffer over the SPI bus.
 *
 * \param[in] data A pointer to the buffer which contains the data to send.
 * \param[in] data_len The number of bytes to send.
 */
void spi_send_data(const uint8_t* data, uint16_t data_len)
{
    do
    {
        uint8_t b = *data++;

        while(!(SPSR & (1 << SPIF)));
        SPDR = b;

    } while(--data_len);

    while(!(SPSR & (1 << SPIF)));
}

/**
 * Receives multiple bytes from the SPI bus and writes them to a buffer.
 *
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] buffer_len The number of bytes to read.
 */
void spi_rec_data(uint8_t* buffer, uint16_t buffer_len)
{
    --buffer;

    do
    {
        SPDR = 0xff;
        ++buffer;

        while(!(SPSR & (1 << SPIF)));
        *buffer = SPDR;

    } while(--buffer_len);
}

/**
 * Switches to the lowest SPI frequency possible.
 */
void spi_low_frequency()
{
	SPCR =	(0<<SPIE) | /* SPI Interrupt Enable */
			(1<<SPE)  | /* SPI Enable */
			(0<<DORD) | /* Data Order: MSB first */
			(1<<MSTR) | /* Master mode */
			(0<<CPOL) | /* Clock Polarity: SCK low when idle */
			(0<<CPHA) | /* Clock Phase: sample on rising SCK edge */
			(1<<SPR1) | /* Clock Frequency: f_OSC / 128 */
			(1<<SPR0);
	SPSR &= ~(1<<SPI2X); /* No Doubled Clock Frequency */
}

/**
 * Switches to the highest SPI frequency possible.
 */
void spi_high_frequency()
{
    SPCR =	(0<<SPIE) | /* SPI Interrupt Enable */
			(1<<SPE)  | /* SPI Enable */
			(0<<DORD) | /* Data Order: MSB first */
			(1<<MSTR) | /* Master mode */
			(0<<CPOL) | /* Clock Polarity: SCK low when idle */
			(0<<CPHA) | /* Clock Phase: sample on rising SCK edge */
			(0<<SPR1) | /* Clock Frequency: f_OSC / 4 */
			(0<<SPR0);
	SPSR |= (1<<SPI2X); /* Doubled Clock Frequency: f_OSC / 2 */
}

/**
 * @}
 * @}
 */

