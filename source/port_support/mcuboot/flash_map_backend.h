/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 /*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
 /*******************************************************************************
* \copyright
*
* (c) 2019, Cypress Semiconductor Corporation
* or a subsidiary of Cypress Semiconductor Corporation. All rights
* reserved.
*
* This software is a port of the open source MCUBoot project.
*
* This file was modified to fit PSoC6-based MCUBoot applications.
*
* Portions of this software, including source code, documentation and related
* materials ("Software"), are owned by Cypress Semiconductor
* Corporation or one of its subsidiaries ("Cypress") and is protected by
* and subject to worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-
* exclusive, non-transferable license to copy, modify, and compile the
* Software source code solely for use in connection with Cypress's
* integrated circuit products. Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO
* WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING,
* BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE. Cypress reserves the right to make
* changes to the Software without notice. Cypress does not assume any
* liability arising out of the application or use of the Software or any
* product or circuit described in the Software. Cypress does not
* authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*
********************************************************************************/
#ifndef __FLASH_MAP_BACKEND_H__
#define __FLASH_MAP_BACKEND_H__

#include "sysflash.h"


#define BOOT_EFLASH      1
#define BOOT_EFILE       2
#define BOOT_EBADIMAGE   3
#define BOOT_EBADVECT    4
#define BOOT_EBADSTATUS  5
#define BOOT_ENOMEM      6
#define BOOT_EBADARGS    7
#define BOOT_EBADVERSION 8

/*
 * Swap type values from mcuboot
 */

/** Attempt to boot the contents of the primary slot. */
#define BOOT_SWAP_TYPE_NONE     1

/*
 * Swap to the secondary slot.
 * Absent a confirm command, revert back on next boot.
 */
#define BOOT_SWAP_TYPE_TEST     2

/*
 * Swap to the secondary slot and permanently switch to booting its contents.
 */
#define BOOT_SWAP_TYPE_PERM     3

/* Swap back to alternate slot.  A confirm changes this state to NONE. */
#define BOOT_SWAP_TYPE_REVERT   4

/* Swap failed because image to be run is not valid */
#define BOOT_SWAP_TYPE_FAIL     5

/* Swapping encountered an unrecoverable error */
#define BOOT_SWAP_TYPE_PANIC    0xff

/* For setting the image_ok flag */
#define BOOT_FLAG_SET       1

#define FLASH_DEVICE_INDEX_MASK                 (0x7F)
#define FLASH_DEVICE_GET_EXT_INDEX(n)           ((n) & FLASH_DEVICE_INDEX_MASK)
#define FLASH_DEVICE_EXTERNAL_FLAG              (0x80)
#define FLASH_DEVICE_INTERNAL_FLASH             (0x7F)
#define FLASH_DEVICE_EXTERNAL_FLASH(index)      (FLASH_DEVICE_EXTERNAL_FLAG | index)

#ifndef CY_BOOT_EXTERNAL_DEVICE_INDEX
/* assume first(one) SMIF0 device is used */
#define CY_BOOT_EXTERNAL_DEVICE_INDEX            (0)
#endif

/* multi-image ? */
#if (MCUBOOT_IMAGE_NUMBER == 1)

#define FLASH_AREA_IMAGE_PRIMARY(x)    (((x) == 0) ?          \
                                         FLASH_AREA_IMG_1_PRIMARY : \
                                         FLASH_AREA_IMG_1_PRIMARY)

#define FLASH_AREA_IMAGE_SECONDARY(x)  (((x) == 0) ?          \
                                         FLASH_AREA_IMG_1_SECONDARY : \
                                         FLASH_AREA_IMG_1_SECONDARY)

#elif (MCUBOOT_IMAGE_NUMBER == 2)

#define FLASH_AREA_IMAGE_PRIMARY(x)    (((x) == 0) ?          \
                                         FLASH_AREA_IMG_1_PRIMARY : \
                                        ((x) == 1) ?          \
                                         FLASH_AREA_IMG_2_PRIMARY : \
                                         255)
#define FLASH_AREA_IMAGE_SECONDARY(x)  (((x) == 0) ?          \
                                         FLASH_AREA_IMG_1_SECONDARY : \
                                        ((x) == 1) ?          \
                                         FLASH_AREA_IMG_2_SECONDARY : \
                                         255)

#else
#warning "Image slot and flash area mapping is not defined"
#endif


#if (defined (CYW20829A0LKML) && MCUBOOT_IMAGE_NUMBER > 1)
#error Multi-image configuration is NOT supported on this platform!
#endif /* (defined(CYW20829) && MCUBOOT_IMAGE_NUMBER > 1) */

/**
 *
 * Provides abstraction of flash regions for type of use.
 * I.e. dude where's my image?
 *
 * System will contain a map which contains flash areas. Every
 * region will contain flash identifier, offset within flash and length.
 *
 * 1. This system map could be in a file within filesystem (Initializer
 * must know/figure out where the filesystem is at).
 * 2. Map could be at fixed location for project (compiled to code)
 * 3. Map could be at specific place in flash (put in place at mfg time).
 *
 * Note that the map you use must be valid for BSP it's for,
 * match the linker scripts when platform executes from flash,
 * and match the target offset specified in download script.
 */
#include <inttypes.h>


#if 0 // MCUBOOT_HAVE_LOGGING

#define BOOT_LOG_ERR(...) printf(__VA_ARGS__)
#define BOOT_LOG_WRN(...) printf(__VA_ARGS__)
#define BOOT_LOG_INF(...) printf(__VA_ARGS__)
#define BOOT_LOG_DBG(...) printf(__VA_ARGS__)
#define BOOT_LOG_SIM(...) printf(__VA_ARGS__)

#else

#define BOOT_LOG_ERR(...)
#define BOOT_LOG_WRN(...)
#define BOOT_LOG_INF(...)
#define BOOT_LOG_DBG(...)
#define BOOT_LOG_SIM(...)

#endif /* MCUBOOT_HAVE_LOGGING */

/**
 * @brief Structure describing an area on a flash device.
 *
 * Multiple flash devices may be available in the system, each of
 * which may have its own areas. For this reason, flash areas track
 * which flash device they are part of.
 */
struct flash_area {
    /**
     * This flash area's ID; unique in the system.
     */
    uint8_t fa_id;

    /**
     * ID of the flash device this area is a part of.
     */
    uint8_t fa_device_id;

    uint16_t pad16;

    /**
     * This area's offset, relative to the beginning of its flash
     * device's storage.
     */
    uint32_t fa_off;

    /**
     * This area's size, in bytes.
     */
    uint32_t fa_size;
};

/*< Opens the area for use. id is one of the `fa_id`s */
int flash_area_open(uint8_t id, const struct flash_area **);
void flash_area_close(const struct flash_area *);
/*< Reads `len` bytes of flash memory at `off` to the buffer at `dst` */
int flash_area_read(const struct flash_area *, uint32_t off, void *dst,
                     uint32_t len);
/*< Writes `len` bytes of flash memory at `off` from the buffer at `src` */
int flash_area_write(const struct flash_area *, uint32_t off,
                     const void *src, uint32_t len);
/*< Erases `len` bytes of flash memory at `off` */
int flash_area_erase(const struct flash_area *, uint32_t off, uint32_t len);

/* writes MAGIC, OK and swap_type */
int flash_area_boot_set_pending(int image, int permanent);

/* writes MAGIC and OK */
int flash_area_boot_set_confirmed(void);


#endif /* __FLASH_MAP_BACKEND_H__ */
