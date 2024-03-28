/*
 * Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/*
 *  Cypress OTA Agent network abstraction for Bluetooth®
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cy_ota_api.h"
#include "cy_ota_log.h"

#ifdef COMPONENT_OTA_BLUETOOTH

#include "cy_ota_internal.h"
#include "cy_ota_ble_secure.h"
#include "cyhal_gpio.h"


/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/

/***********************************************************************
 *
 * Macros
 *
 **********************************************************************/
#ifndef CY_DS_SIZE
#define CY_DS_SIZE           0x3C0000
#endif

#ifndef FLASH_AREA_IMG_1_SECONDARY_SIZE
#define UPGRADE_SLOT_SIZE    CY_DS_SIZE
#else
#define UPGRADE_SLOT_SIZE    FLASH_AREA_IMG_1_SECONDARY_SIZE
#endif

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/***********************************************************************
 *
 * Data & Variables
 *
 **********************************************************************/

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

#ifndef COMPONENT_OTA_BLUETOOTH_SECURE
static uint32_t cy_ota_ble_crc32_update(uint32_t prev_crc32, const uint8_t* buffer, uint16_t buffer_length)
{
    uint32_t crc32 = ~prev_crc32;
    uint16_t i;

    for(i = 0; i < buffer_length; i++)
    {
        uint16_t j;

        crc32 ^= *buffer;
        buffer++;

        for(j = 0; j < 8; j++)
        {
            if(crc32 & (uint32_t)0x1U)
            {
                crc32 = (crc32 >> 1) ^ 0xEDB88320U;
            }
            else
            {
                crc32 = (crc32 >> 1);
            }
        }
    }

    return ~crc32;
}
#endif

cy_rslt_t cy_ota_ble_validate_network_params(const cy_ota_network_params_t *network_params)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Nothing to check at the moment */

    return result;
}


cy_rslt_t cy_ota_ble_download_prepare(cy_ota_context_ptr ctx_ptr)
{
    cy_rslt_t               result;
    cy_ota_context_t        *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s()\n", __func__);

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    cy_ota_ble_secure_signature_init(ctx_ptr);
#endif
    ota_ctx->ble.file_bytes_written = 0;

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_STORAGE_OPEN);

    /* Call Open Storage - this erases Secondary Slot for storing downloaded OTA Image */
    result = ota_ctx->storage_iface.ota_file_open(&(ota_ctx->ota_storage_context));
    if(result != CY_RSLT_SUCCESS)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "     OTA platform storage open API FAILED\n");
        return CY_RSLT_OTA_ERROR_BLE_STORAGE;
    }

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_AGENT_WAITING);
    return result;
}

cy_rslt_t cy_ota_ble_download(cy_ota_context_ptr ctx_ptr, uint32_t update_file_size)
{
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s()\n", __func__);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "OTA Update file size : 0x%lx (%ld)\n", update_file_size, update_file_size);
    ota_ctx->ota_storage_context.total_image_size = update_file_size;
    ota_ctx->ota_storage_context.total_bytes_written = 0;
    ota_ctx->ble.crc32 = CRC32_INITIAL_VALUE;
    ota_ctx->ble.percent = 0;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "Update OTA state to CY_OTA_STATE_START_UPDATE\n");
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_START_UPDATE);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_ota_ble_download_write(cy_ota_context_ptr ctx_ptr, uint8_t *data_buf, uint16_t len, uint16_t offset)
{
    cy_ota_storage_write_info_t     chunk_info;
    cy_rslt_t                       result;
    uint32_t                        full_write_size;
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s()\n", __func__);

    /* prepare to call ota library write routine */
    (void)memset(&chunk_info, 0x00, sizeof(chunk_info));
    chunk_info.buffer = data_buf;
    chunk_info.offset = offset;
    chunk_info.size   = len;
    full_write_size   = chunk_info.size;

    /*
     * Don't require App to keep track of offset into the Slot
     * We also handle the offset handed in, in case we are pointing into a buffer
     * rather than the start of the buffer
     */
    chunk_info.buffer = &chunk_info.buffer[chunk_info.offset];         /* Handle offset into the buffer given us */
    chunk_info.offset = ota_ctx->ota_storage_context.last_offset;      /* Need to set the offset into Secondary Slot properly for the write */

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_STORAGE_WRITE);

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    if(chunk_info.offset + chunk_info.size > UPGRADE_SLOT_SIZE)
    {
        chunk_info.size = UPGRADE_SLOT_SIZE - chunk_info.offset;
    }
#endif

    if(chunk_info.size > 0)
    {
        result = ota_ctx->storage_iface.ota_file_write(&(ota_ctx->ota_storage_context), &chunk_info);
        if(result != CY_RSLT_SUCCESS)
        {
            cy_rtos_setbits_event(&ota_ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_FAIL, 0);

            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "     OTA platform storage block write API FAILED : 0x%lx \n", result);
            return CY_RSLT_OTA_ERROR_BLE_STORAGE;
        }
    }

    /*
     * Make sure the chunk info size is correct in case we needed to strip
     * out the trailing signature for secure BLE.
     */

    chunk_info.size = full_write_size;

    /* update the stats */
    ota_ctx->ota_storage_context.total_bytes_written   += chunk_info.size;      /* Total bytes written to flash         */
    ota_ctx->ota_storage_context.last_size              = chunk_info.size;      /* last data size received              */
    ota_ctx->ota_storage_context.last_offset           += chunk_info.size;      /* Keep track of offset into the Slot   */
    if(ota_ctx->ota_storage_context.total_image_size > 0)
    {
        ota_ctx->ble.percent = ((uint32_t)100 * ota_ctx->ota_storage_context.total_bytes_written) / ota_ctx->ota_storage_context.total_image_size;
    }

    /* update crc or secure signature as we pull in the data */
#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    cy_ota_ble_secure_signature_update(ctx_ptr, data_buf, offset, len);
#else
    ota_ctx->ble.crc32 = cy_ota_ble_crc32_update(ota_ctx->ble.crc32, (const uint8_t*)((uint32_t)data_buf + offset), len);
#endif
    ota_ctx->ble.file_bytes_written += chunk_info.size;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "   Downloaded 0x%lx of 0x%lx (%d%%)\n", ota_ctx->ota_storage_context.total_bytes_written, ota_ctx->ota_storage_context.total_image_size, ota_ctx->ble.percent);
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_DATA_DOWNLOAD);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_ota_ble_download_verify(cy_ota_context_ptr ctx_ptr, uint32_t final_crc32, bool verify_crc_or_signature)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_VERIFY);

    if(verify_crc_or_signature)
    {
#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
        result = cy_ota_ble_secure_signature_verify(ctx_ptr);
        if(result == CY_RSLT_SUCCESS)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "     Bluetooth(r) Secure Signature Verification Succeeded!\n");
        }
#else
        /* non- secure here, check CRC */
        ota_ctx->ble.received_crc32 = final_crc32;
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "Calculated CRC : 0x%lx\n", final_crc32);
        if(ota_ctx->ble.crc32 != ota_ctx->ble.received_crc32)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "     check CRC FAILED 0x%lx != 0x%lx\n", ota_ctx->ble.crc32, ota_ctx->ble.received_crc32);
            result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
        }
        else
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "     Bluetooth(r) CRC Verification Succeeded!\n");
            result = CY_RSLT_SUCCESS;
        }
#endif
    }

    if(result != CY_RSLT_SUCCESS)
    {
        cy_ota_set_state(ota_ctx, CY_OTA_STATE_EXITING);
    }
    else
    {
        /* Call user image verify callback API. */
        result = ota_ctx->storage_iface.ota_file_verify(&(ota_ctx->ota_storage_context));
        if(result != CY_RSLT_SUCCESS)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "\nStorage verify API failed: 0x%lx", result);
            result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
            cy_ota_set_state(ota_ctx, CY_OTA_STATE_EXITING);
        }
    }

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_OTA_COMPLETE);

    return result;
}

cy_rslt_t cy_ota_ble_download_abort(cy_ota_context_ptr ctx_ptr)
{
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s(): Set state\n", __func__);
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_AGENT_WAITING);

    return CY_RSLT_SUCCESS;
}

#endif 	/* COMPONENT_OTA_BLUETOOTH */
