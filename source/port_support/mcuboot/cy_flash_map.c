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
/***************************************************************************//**
* \file cy_flash_map.c
* \version 1.0
*
* \brief
*  This is the source file of flash driver adaptation layer between PSoC6
*  and standard MCUBoot code.
*
********************************************************************************
* \copyright
*
* (c) 2019, Cypress Semiconductor Corporation
* or a subsidiary of Cypress Semiconductor Corporation. All rights
* reserved.
*
* This software, including source code, documentation and related
* materials ("Software"), is owned by Cypress Semiconductor
* Corporation or one of its subsidiaries ("Cypress") and is protected by
* and subject to worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-
* exclusive, non-transferable license to copy, modify, and compile the
* Software source code solely for use in connection with Cypress?s
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
******************************************************************************/

/*****************************************************************************
 *
 * This is a modified version that only includes minimal functionality needed by
 * Infineon ota-update library.
 *
******************************************************************************/

/*
 * The defines used can be generated by a JSON file, the files are located in configs/flashmap.
 * Add to the make command line
 * make build FLASH_MAP=$(SEARCH_ota-update/configs/flashmap/<flash_map.json>
 *
 * FLASH_AREA_BOOTLOADER_DEV_ID
 * FLASH_AREA_BOOTLOADER_START
 * FLASH_AREA_BOOTLOADER_SIZE
 * FLASH_AREA_IMG_1_PRIMARY_DEV_ID
 * FLASH_AREA_IMG_1_PRIMARY_START
 * FLASH_AREA_IMG_1_PRIMARY_SIZE
 * FLASH_AREA_IMG_1_SECONDARY_DEV_ID
 * FLASH_AREA_IMG_1_SECONDARY_START
 * FLASH_AREA_IMG_1_SECONDARY_SIZE
 * FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID
 * FLASH_AREA_IMAGE_SWAP_STATUS_START
 * FLASH_AREA_IMAGE_SWAP_STATUS_SIZE
 * FLASH_AREA_IMAGE_SCRATCH_DEV_ID
 * FLASH_AREA_IMAGE_SCRATCH_START
 * FLASH_AREA_IMAGE_SCRATCH_SIZE
 * MCUBOOT_IMAGE_NUMBER
 * MCUBOOT_MAX_IMG_SECTORS
 */



#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>     /* for memcpy */
#include "cy_pdl.h"
#if defined(PSOC_062_2M) || defined(PSOC_062_1M) || defined(PSOC_062_512K) || defined(PSOC_063_1M) || defined(PSOC_064_2M)
#include "flash_qspi.h"
#include "cy_flash_psoc6.h"
#endif

#include "flash_map_backend.h"

#include "ota_serial_flash.h"

/* This is the value of internal flash bytes after an erase */
#define CY_BOOT_INTERNAL_FLASH_ERASE_VALUE      (0x00)
/* This is the value of external flash bytes after an erase */
#define CY_BOOT_EXTERNAL_FLASH_ERASE_VALUE      (0xff)

/* For CYW20829 this needs to be 256. Tested OK */
#if defined(CYW20829)
    #define BOOT_MAX_ALIGN          256
#endif

#if defined(PSOC_062_2M) || defined(PSOC_062_1M) || defined(PSOC_062_512K) || defined(PSOC_063_1M) || defined(PSOC_064_2M)
    #define BOOT_MAX_ALIGN          512
#endif

#ifndef  BOOT_MAX_ALIGN
#warning "No MAX_BOOT_ALIGN set, defaulting to 8"
#define BOOT_MAX_ALIGN          8
#endif

/* alignment within the MCUBoot Trailer */
#define BOOT_TRAILER_ALIGN		8


/* This is not actually used by mcuboot's code but can be used by apps
 * when attempting to read/write a trailer.
	struct image_trailer {
		uint8_t swap_type;
		uint8_t pad1[BOOT_TRAILER_ALIGN - 1];
		uint8_t copy_done;
		uint8_t pad2[BOOT_TRAILER_ALIGN - 1];
		uint8_t image_ok;
		uint8_t pad3[BOOT_TRAILER_ALIGN - 1];
		uint8_t magic[16];
	};
*/


/* Construct the swap_info field from swap type and image number */
#define BOOT_SET_SWAP_INFO(swap_info, image, type)  {                          \
                                                    assert((image) < 0xF);     \
                                                    assert((type)  < 0xF);     \
                                                    (swap_info) = (image) << 4 \
                                                                | (type);      \
                                                    }

static const uint32_t boot_img_magic[] = {
    0xf395c277,
    0x7fefd260,
    0x0f505235,
    0x8079b62c,
};

/* size of the boot_image_magic in mcuboot slot */
#define BOOT_MAGIC_SZ   (sizeof boot_img_magic)

uint8_t flash_area_erased_val(const struct flash_area *fap);

static struct flash_area flash_areas[] = {
    {
        .fa_id        = FLASH_AREA_BOOTLOADER,
        .fa_device_id = FLASH_AREA_BOOTLOADER_DEV_ID,
        .fa_off       = FLASH_AREA_BOOTLOADER_START,
        .fa_size      = FLASH_AREA_BOOTLOADER_SIZE,
    },
    {
        .fa_id        = FLASH_AREA_IMG_1_PRIMARY,
        .fa_device_id = FLASH_AREA_IMG_1_PRIMARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_1_PRIMARY_START,
        .fa_size      = FLASH_AREA_IMG_1_PRIMARY_SIZE,
    },
    {
        .fa_id        = FLASH_AREA_IMG_1_SECONDARY,
        .fa_device_id = FLASH_AREA_IMG_1_SECONDARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_1_SECONDARY_START,
        .fa_size      = FLASH_AREA_IMG_1_SECONDARY_SIZE,
    }
#ifdef FLASH_AREA_IMAGE_SCRATCH_DEV_ID
    ,{
        .fa_id        = FLASH_AREA_IMAGE_SCRATCH,
        .fa_device_id = FLASH_AREA_IMAGE_SCRATCH_DEV_ID,
        .fa_off       = FLASH_AREA_IMAGE_SCRATCH_START,
        .fa_size      = FLASH_AREA_IMAGE_SCRATCH_SIZE,
    }
#endif
#if MCUBOOT_IMAGE_NUMBER >= 2
    ,{
        .fa_id        = FLASH_AREA_IMG_2_PRIMARY,
        .fa_device_id = FLASH_AREA_IMG_2_PRIMARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_2_PRIMARY_START,
        .fa_size      = FLASH_AREA_IMG_2_PRIMARY_SIZE,
    },
    {
        .fa_id        = FLASH_AREA_IMG_2_SECONDARY,
        .fa_device_id = FLASH_AREA_IMG_2_SECONDARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_2_SECONDARY_START,
        .fa_size      = FLASH_AREA_IMG_2_SECONDARY_SIZE,
    }
#endif
#ifdef FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID
    ,{
        .fa_id        = FLASH_AREA_IMAGE_SWAP_STATUS,
        .fa_device_id = FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID,
        .fa_off       = FLASH_AREA_IMAGE_SWAP_STATUS_START,
        .fa_size      = FLASH_AREA_IMAGE_SWAP_STATUS_SIZE,
    }
#endif
#if MCUBOOT_IMAGE_NUMBER >= 3
    ,{
        .fa_id        = FLASH_AREA_IMG_3_PRIMARY,
        .fa_device_id = FLASH_AREA_IMG_3_PRIMARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_3_PRIMARY_START,
        .fa_size      = FLASH_AREA_IMG_3_PRIMARY_SIZE,
    },
    {
        .fa_id        = FLASH_AREA_IMG_3_SECONDARY,
        .fa_device_id = FLASH_AREA_IMG_3_SECONDARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_3_SECONDARY_START,
        .fa_size      = FLASH_AREA_IMG_3_SECONDARY_SIZE,
    }
#endif

#if MCUBOOT_IMAGE_NUMBER == 4
    ,{
        .fa_id        = FLASH_AREA_IMG_4_PRIMARY,
        .fa_device_id = FLASH_AREA_IMG_4_PRIMARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_4_PRIMARY_START,
        .fa_size      = FLASH_AREA_IMG_4_PRIMARY_SIZE,
    },
    {
        .fa_id        = FLASH_AREA_IMG_4_SECONDARY,
        .fa_device_id = FLASH_AREA_IMG_4_SECONDARY_DEV_ID,
        .fa_off       = FLASH_AREA_IMG_4_SECONDARY_START,
        .fa_size      = FLASH_AREA_IMG_4_SECONDARY_SIZE,
    }
#endif

};

struct flash_area *boot_area_descs[] = {
    &flash_areas[0U],
    &flash_areas[1U],
    &flash_areas[2U],
#ifdef FLASH_AREA_IMAGE_SCRATCH_START
    &flash_areas[3U],
#endif
#if MCUBOOT_IMAGE_NUMBER >= 2
    &flash_areas[4U],
    &flash_areas[5U],
#endif
#ifdef FLASH_AREA_IMAGE_SWAP_STATUS_START
    &flash_areas[6U],
#endif
    #if MCUBOOT_IMAGE_NUMBER >= 3
    &flash_areas[7U],
    &flash_areas[8U],
#endif
#if MCUBOOT_IMAGE_NUMBER >= 4
    &flash_areas[9U],
    &flash_areas[10U],
#endif
    NULL
};

/* Uncomment to print flash area info when area is opened */
//#define DEBUG_PRINT_OPEN_AREA
#ifndef DEBUG_PRINT_OPEN_AREA
#define DEBUG_PRINT_FLASH_AREA(fa)
#else
void print_flash_area_desc(const struct flash_area *fa)
{
    printf("\nMCUBoot flash area:\n");
    printf("      fa_id:  %d\r\n", fa->fa_id);
    printf("  device_id:  %d\r\n", fa->fa_device_id);
    printf("     fa_off:  0x%08lx\r\n", fa->fa_off);
    printf("    fa_size:  0x%08lx\r\n", fa->fa_size);
    printf("  erase_val:  0x%02x\r\n", flash_area_erased_val(fa));
    printf("\r\n");
}

 void print_all_flash_area_descs(void)
 {
     int i;

     printf("\r\nMCUBoot flash areas:\r\n");
     for (i = 0; boot_area_descs[i] != NULL; i++)
     {
         print_flash_area_desc(boot_area_descs[i]);
     }
 }

#define DEBUG_PRINT_FLASH_AREA(fa)  print_flash_area_desc(fa)
#define DEBUG_PRINT_FLASH_AREAS()   print_all_flash_area_descs
#endif


 uint8_t flash_area_erased_val(const struct flash_area *fap)
 {
     int ret = 0;

     if (fap->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH)
     {
         ret = CY_BOOT_INTERNAL_FLASH_ERASE_VALUE ;
     }
 #ifdef OTA_USE_EXTERNAL_FLASH
     else if ((fap->fa_device_id & FLASH_DEVICE_EXTERNAL_FLAG) == FLASH_DEVICE_EXTERNAL_FLAG)
     {
         ret = CY_BOOT_EXTERNAL_FLASH_ERASE_VALUE;
     }
 #endif
     else
     {
         assert(false) ;
     }

     return ret ;
 }

 /*
* Opens the area for use. id is one of the `fa_id`s
*/
int flash_area_open(uint8_t id, const struct flash_area **fa)
{
    int ret = -1;
    uint32_t i = 0;

    if (NULL != fa) {
        while(NULL != boot_area_descs[i])
        {
        if(id == boot_area_descs[i]->fa_id)
        {
        *fa = boot_area_descs[i];
                ret = 0;
                DEBUG_PRINT_FLASH_AREA(*fa);    /* enable above for debugging */
                break;
        }
            i++;
        }
    }

    return ret;
}

/*
* Clear pointer to flash area fa
*/
void flash_area_close(const struct flash_area *fa)
{
    (void)fa; /* Nothing to do there */
}

/*
* Reads `len` bytes of flash memory at `off` to the buffer at `dst`
*/
int flash_area_read(const struct flash_area *fa, uint32_t off, void *dst,
                     uint32_t len)
{
    int rc = 0;
    size_t addr;

    /* check if requested offset not less then flash area (fa) start */
    if ( (NULL == fa) || (NULL == dst) ||
         (off > fa->fa_size || len > fa->fa_size || off + len > fa->fa_size) )
    {
        return BOOT_EBADARGS;
    }

    /* Add base of flash area and offset within the flash area */
    addr = fa->fa_off + off;

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH)
    {
#if defined(CYW20829)
        /* CYW20829 does not have internal flash - check your flash layout JSON file!*/
        return BOOT_EBADARGS;
#endif
#if defined(PSOC_062_2M) || defined(PSOC_062_1M) || defined(PSOC_062_512K) || defined(PSOC_063_1M) || defined(PSOC_064_2M)
        rc = psoc6_flash_read(addr, dst, len);
#endif
    }
#ifdef OTA_USE_EXTERNAL_FLASH
    else if ((fa->fa_device_id & FLASH_DEVICE_EXTERNAL_FLAG) == FLASH_DEVICE_EXTERNAL_FLAG)
    {
        rc = ota_smif_read(addr, dst, len);
    }
#endif
    else
    {
        /* incorrect/non-existing flash device id */
        rc = -1;
    }

    if (rc != 0) {
        BOOT_LOG_ERR("Flash area read error, rc = %d", (int)rc);
    }
    return rc;
}

/*< Writes `len` bytes of flash memory at `off` from the buffer at `src` */
int flash_area_write(const struct flash_area *fa, uint32_t off,
                     const void *src, uint32_t len)
{
    int rc = 0;
    size_t addr;

    /* check if requested offset not less then flash area (fa) start */
    if ( (NULL == fa) || (NULL == src) ||
         (off > fa->fa_size || len > fa->fa_size || off + len > fa->fa_size) )
    {
        return BOOT_EBADARGS;
    }

    /* Add base of flash area and offset within the flash area */
    addr = fa->fa_off + off;

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH)
    {
#if defined(CYW20829)
        /* CYW20829 does not have internal flash - check your flash layout JSON file!*/
        return BOOT_EBADARGS;
#endif
#if defined(PSOC_062_2M) || defined(PSOC_062_1M) || defined(PSOC_062_512K) || defined(PSOC_063_1M) || defined(PSOC_064_2M)
        rc = psoc6_flash_write(addr, src, len);
#endif
    }
#ifdef OTA_USE_EXTERNAL_FLASH
    else if ((fa->fa_device_id & FLASH_DEVICE_EXTERNAL_FLAG) == FLASH_DEVICE_EXTERNAL_FLAG)
    {
        rc = ota_smif_write(addr, src, len);
    }
#endif
    else
    {
        /* incorrect/non-existing flash device id */
        rc = -1;
    }

    return rc;
}

/*< Erases `len` bytes of flash memory at `off` */
int flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    int rc = 0;
    size_t addr;

    assert(off < fa->fa_off);
    assert(off + len < fa->fa_off + fa->fa_size);
    /* Add base of flash area and offset within the flash area */
    addr = fa->fa_off + off;

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH)
    {
#if defined(CYW20829)
        /* CYW20829 does not have internal flash - check your flash layout JSON file!*/
        return BOOT_EBADARGS;
#endif
#if defined(PSOC_062_2M) || defined(PSOC_062_1M) || defined(PSOC_062_512K) || defined(PSOC_063_1M) || defined(PSOC_064_2M)
        rc = psoc6_flash_erase(addr, len);
#endif
    }
#ifdef OTA_USE_EXTERNAL_FLASH
    else if ((fa->fa_device_id & FLASH_DEVICE_EXTERNAL_FLAG) == FLASH_DEVICE_EXTERNAL_FLAG)
    {
        rc = ota_smif_erase(addr, len);
    }
#endif /* OTA_USE_EXTERNAL_FLASH */
    else
    {
        /* incorrect/non-existing flash device id */
        rc = -1;
    }
    return rc;
}

static inline uint32_t
boot_magic_off(const struct flash_area *fap)
{
    return fap->fa_size - BOOT_MAGIC_SZ;
}

static inline uint32_t
boot_image_ok_off(const struct flash_area *fap)
{
    return boot_magic_off(fap) - BOOT_TRAILER_ALIGN;
}
static inline uint32_t
boot_copy_done_off(const struct flash_area *fap)
{
    return boot_image_ok_off(fap) - BOOT_TRAILER_ALIGN;
}

static inline uint32_t
boot_swap_info_off(const struct flash_area *fap)
{
    return boot_copy_done_off(fap) - BOOT_TRAILER_ALIGN;
}

/*< Returns this `flash_area`s alignment */
size_t flash_area_align(const struct flash_area *fa)
{
    size_t rc = 0u; /* error code (alignment cannot be zero) */

    if (NULL != fa)
    {
        if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH)
        {
            rc = CY_FLASH_ALIGN;
        }
#ifdef OTA_USE_EXTERNAL_FLASH
        else if ((fa->fa_device_id & FLASH_DEVICE_EXTERNAL_FLAG) == FLASH_DEVICE_EXTERNAL_FLAG)
        {
            rc = ota_smif_get_prog_size(0);
        }
#endif /* OTA_USE_EXTERNAL_FLASH */
        else
        {
            /* Not valid - alignment cannot be 0 */
        }
    }

    return rc;
}

/**
 * Write trailer data; status bytes, swap_size, etc
 *
 * @returns 0 on success, != 0 on error.
 */
int
boot_write_trailer(const struct flash_area *fap, uint32_t off,
        const uint8_t *inbuf, uint8_t inlen)
{
    uint8_t buf[BOOT_MAX_ALIGN];
    size_t align;
    uint8_t erased_val;
    int rc;

    align = flash_area_align(fap);
    if (align == 0u) {
        return BOOT_EFLASH;
    }
    align = (inlen + align - 1) & ~(align - 1);
    if (align > BOOT_MAX_ALIGN) {
//        printf("%d:%s() align 0x%x > 0x%x BOOT_MAX_ALIGN , ret -1\n", __LINE__, __func__, align, BOOT_MAX_ALIGN);
        return -1;
    }
    erased_val = flash_area_erased_val(fap);

    memcpy(buf, inbuf, inlen);
    memset(&buf[inlen], erased_val, align - inlen);

    if (off + align > fap->fa_size)
    {
	align = fap->fa_size - off;
    }
    rc = flash_area_write(fap, off, buf, align);
//    printf("%d:%s() flash_area_write(fap, 0x%lx, %p, 0x%lx) , ret %d\n", __LINE__, __func__, off, buf, align, rc);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    return 0;
}

int
boot_write_magic(const struct flash_area *fap)
{
    uint8_t magic[BOOT_MAGIC_SZ];
    uint32_t off;
    int rc;

    /*
     * For XIP builds, boot_img_magic may end up stored in external flash.
     * Copy the data to a temporary buffer to avoid trying to access
     * an external flash buffer while using ram SMIF write routines..
     */

    memcpy(magic, boot_img_magic, BOOT_MAGIC_SZ);

    off = boot_magic_off(fap);
    rc = flash_area_write(fap, off, magic, BOOT_MAGIC_SZ);
//    printf("%s() fap->off 0x%lx off 0x%lx rc %d\n", __func__, fap->fa_off, off, rc);
    if (rc != 0)
    {
        return BOOT_EFLASH;
    }

    return 0;
}

int
boot_write_image_ok(const struct flash_area *fap)
{
    uint32_t off;
    uint8_t  buff[2];
    int rc;

    off = boot_image_ok_off(fap);
    buff[0] = BOOT_FLAG_SET;
    rc = flash_area_write(fap, off, buff, 1);
//    printf("%s() fap->off 0x%lx off 0x%lx rc %d\n", __func__, fap->fa_off, off, rc);
    if (rc != 0)
    {
        return BOOT_EFLASH;
    }

    return 0;
}

/**
 * Writes the specified value to the `swap-type` field of an image trailer.
 * This value is persisted so that the boot loader knows what swap operation to
 * resume in case of an unexpected reset.
 */
int
boot_write_swap_info(const struct flash_area *fap, uint8_t swap_type,
                     uint8_t image_num)
{
    uint32_t off;
    uint8_t swap_info;
    int     rc;

    BOOT_SET_SWAP_INFO(swap_info, image_num, swap_type);
    off = boot_swap_info_off(fap);
    rc = boot_write_trailer(fap, off, (const uint8_t *) &swap_info, 1);
//    printf("%s() fap->off 0x%lx off 0x%lx rc %d\n", __func__, fap->fa_off, off, rc);
    if (rc != 0)
    {
        return BOOT_EFLASH;
    }

    return 0;
}

/**
 * Marks the image in the secondary slot as pending.  On the next reboot,
 * the system will perform a one-time boot of the the secondary slot image.
 *
 * @param image             0 = Secondary_Slot_1, 1 = Secondary_Slot_2
 * @param permanent         Whether the image should be used permanently or
 *                              only tested once:
 *                                  0=run image once, then confirm or revert.
 *                                  1=run image forever.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
flash_area_boot_set_pending(int image, int permanent)
{
    const struct flash_area *fap;
    int                     rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_SECONDARY(image), &fap);
    if (rc == 0)
    {
        rc = boot_write_magic(fap);
//        printf("%s() boot_write_magic(fap) returned %d\n", __func__, rc);

        /*
         * Writing trailer flags doesn't work properly for internal flash. That's OK
         * because writing the magic does work and that's enough to trigger the update.
         */
        if ( (fap->fa_device_id & FLASH_DEVICE_EXTERNAL_FLAG) == FLASH_DEVICE_EXTERNAL_FLAG)
        {
            uint8_t swap_type;
            if (rc == 0 && permanent) {
                rc = boot_write_image_ok(fap);
//                printf("%s() boot_write_image_ok(fap) returned %d\n", __func__, rc);
            }

            if (rc == 0) {
                if (permanent) {
                    swap_type = BOOT_SWAP_TYPE_PERM;
                } else {
                    swap_type = BOOT_SWAP_TYPE_TEST;
                }
                rc = boot_write_swap_info(fap, swap_type, 0);
//                printf("%s() boot_write_swap_info(fap) returned %d\n", __func__, rc);
            }
        }

        flash_area_close(fap);
    }

    return rc;
}

/**
 * Marks the image in the primary slot as confirmed.  The system will continue
 * booting into the image in the primary slot until told to boot from a
 * different slot.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
flash_area_boot_set_confirmed(void)
{
    const struct flash_area *fap;
    int                     rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(0), &fap);
    if (rc == 0)
    {
        rc = boot_write_magic(fap);
//        printf("%s() boot_write_magic() returned %d\n", __func__, rc);
        if (rc == 0)
        {
            rc = boot_write_image_ok(fap);
//            printf("boot_write_image_ok() returned %d\n", rc);
        }
        flash_area_close(fap);
    }

    return rc;
}
