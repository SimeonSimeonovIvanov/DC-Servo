
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef FAT16_CONFIG_H
#define FAT16_CONFIG_H

/**
 * \addtogroup sd
 *
 * @{
 */
/**
 * \addtogroup fat16
 *
 * @{
 */
/**
 * \file
 * FAT16 configuration (license: GPLv2 or LGPLv2.1)
 */

/**
 * \ingroup fat16_config
 * Controls FAT16 write support.
 *
 * Set to 1 to enable FAT16 write support, set to 0 to disable it.
 */
#define FAT16_WRITE_SUPPORT 1

/**
 * \ingroup fat16_config
 * Controls FAT16 date and time support.
 * 
 * Set to 1 to enable FAT16 date and time stamping support.
 */
#define FAT16_DATETIME_SUPPORT 0

/**
 * \ingroup fat16_config
 * Determines the function used for retrieving current date and time.
 *
 * Define this to the function call which shall be used to retrieve
 * current date and time.
 *
 * \note Used only when FAT16_DATETIME_SUPPORT is 1.
 *
 * \param[out] year Pointer to a \c uint16_t which receives the current year.
 * \param[out] month Pointer to a \c uint8_t which receives the current month.
 * \param[out] day Pointer to a \c uint8_t which receives the current day.
 * \param[out] hour Pointer to a \c uint8_t which receives the current hour.
 * \param[out] min Pointer to a \c uint8_t which receives the current minute.
 * \param[out] sec Pointer to a \c uint8_t which receives the current sec.
 */
#define fat16_get_datetime(year, month, day, hour, min, sec)

/**
 * \ingroup fat16_config
 * Maximum number of filesystem handles.
 */
#define FAT16_FS_COUNT 1

/**
 * \ingroup fat16_config
 * Maximum number of file handles.
 */
#define FAT16_FILE_COUNT 5

/**
 * \ingroup fat16_config
 * Maximum number of directory handles.
 */
#define FAT16_DIR_COUNT 6

/**
 * @}
 * @}
 */

#endif

