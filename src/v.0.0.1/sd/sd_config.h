
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef SD_CONFIG_H
#define SD_CONFIG_H

/**
 * \addtogroup sd
 *
 * @{
 */
/**
 * \addtogroup config MMC/SD configuration
 *
 * Compile time configuration common to all MMC/SD modules.
 *
 * @{
 */

/**
 * \file
 * Common configuration used by all MMC/SD modules (license: GPLv2 or LGPLv2.1)
 *
 * \note This file contains only configuration items relevant to
 * all MMC/SD implementation files. For module specific configuration
 * options, please see the files fat16_config.h, partition_config.h
 * and sd_raw_config.h.
 */

/**
 * Controls allocation of memory.
 *
 * Set to 1 to use malloc()/free() for allocation of structures
 * like file and directory handles, set to 0 to use pre-allocated
 * fixed-size handle arrays.
 */
#define USE_DYNAMIC_MEMORY 1

/**
 * @}
 * @}
 */

#endif

