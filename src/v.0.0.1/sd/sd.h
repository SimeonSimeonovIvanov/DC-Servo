
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SD_H
#define SD_H

#include <stdint.h>

/**
 * \addtogroup sd
 *
 * @{
 */
/**
 * \file
 * MMC/SD handling header (license: GPLv2)
 *
 * \author Roland Riegel
 */

#define SD_ERROR_NONE 0
#define SD_ERROR_INIT -1
#define SD_ERROR_PARTITION -2
#define SD_ERROR_FS -3
#define SD_ERROR_ROOTDIR -4

struct fat16_fs_struct;
struct fat16_dir_struct;
struct fat16_dir_entry_struct;
struct fat16_file_struct;

int8_t sd_open();
void sd_close();

struct fat16_fs_struct* sd_get_fs();
struct fat16_dir_struct* sd_get_root_dir();

uint8_t sd_find_file_in_dir(struct fat16_dir_struct* dd, const char* name, struct fat16_dir_entry_struct* dir_entry);
struct fat16_file_struct* sd_open_file_in_dir(struct fat16_dir_struct* dd, const char* name);

/**
 * @}
 */

#endif

