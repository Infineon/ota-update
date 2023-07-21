/*
 * Copyright (c) 2020 Cypress Semiconductor Corporation
 * Copyright (c) 2022 Infineon Technologies AG
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

#ifndef SYSFLASH_H
#define SYSFLASH_H

#include <stdint.h>
#include "cy_syslib.h"

#ifndef MCUBOOT_IMAGE_NUMBER
#ifdef MCUBootApp
#warning Undefined MCUBOOT_IMAGE_NUMBER. Assuming 1 (single-image).
#endif /* MCUBootApp */
#define MCUBOOT_IMAGE_NUMBER 1
#endif /* MCUBOOT_IMAGE_NUMBER */

#if (MCUBOOT_IMAGE_NUMBER < 1 || MCUBOOT_IMAGE_NUMBER > 4)
#error Unsupported MCUBOOT_IMAGE_NUMBER. Set it to between 1 and 4.
#endif /* (MCUBOOT_IMAGE_NUMBER < 1 || MCUBOOT_IMAGE_NUMBER > 4) */

#define FLASH_AREA_BOOTLOADER        ( 0u)

#define FLASH_AREA_IMG_1_PRIMARY     ( 1u)
#define FLASH_AREA_IMG_1_SECONDARY   ( 2u)

#define FLASH_AREA_IMAGE_SCRATCH     ( 3u)

#if MCUBOOT_IMAGE_NUMBER >= 2
#define FLASH_AREA_IMG_2_PRIMARY     ( 4u)
#define FLASH_AREA_IMG_2_SECONDARY   ( 5u)
#endif /* MCUBOOT_IMAGE_NUMBER >= 2 */

#define FLASH_AREA_IMAGE_SWAP_STATUS ( 7u)

#if MCUBOOT_IMAGE_NUMBER >= 3
#define FLASH_AREA_IMG_3_PRIMARY     ( 8u)
#define FLASH_AREA_IMG_3_SECONDARY   ( 9u)
#endif /* MCUBOOT_IMAGE_NUMBER >= 3 */

#if MCUBOOT_IMAGE_NUMBER == 4
#define FLASH_AREA_IMG_4_PRIMARY     (10u)
#define FLASH_AREA_IMG_4_SECONDARY   (11u)
#endif /* MCUBOOT_IMAGE_NUMBER == 4 */

#define FLASH_AREA_ERROR             255u  /* Invalid flash area */

#define CY_SMIF_BASE_MEM_OFFSET             CY_XIP_BASE

/* This defines if External Flash (SMIF) will be used for Upgrade Slots */
#ifdef CYW20829
#define CY_FLASH_BASE                       CY_XIP_BASE /* Override value in /mtb-pdl-cat1/devices/COMPONENT_CAT1A/include/cy_device_common.h for CYW20829 */
#define CY_FLASH_SIZEOF_ROW                 4096u       /* Override value in /mtb-pdl-cat1/devices/COMPONENT_CAT1A/include/cy_device_common.h for CYW20829 */
#endif /* CYW20829 */

#define CY_FLASH_ALIGN                      (CY_FLASH_SIZEOF_ROW)
#define CY_FLASH_DEVICE_BASE                (CY_FLASH_BASE)

#endif /* SYSFLASH_H */
