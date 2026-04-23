/*
 * (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
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

static uint32_t cy_ota_update_get_calculated_crc32(uint32_t prev_crc32, const uint8_t* buffer, uint16_t buffer_length)
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

cy_rslt_t cy_ota_ble_validate_network_params(const cy_ota_network_params_t *network_params)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Nothing to check at the moment */

    return result;
}


cy_rslt_t cy_ota_update_prepare(cy_ota_context_ptr ctx_ptr)
{
    cy_rslt_t               result;
    cy_ota_context_t        *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s()\n", __func__);

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

cy_rslt_t cy_ota_update_download_start(cy_ota_context_ptr ctx_ptr, uint32_t update_file_size)
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

cy_rslt_t cy_ota_update_write(cy_ota_context_ptr ctx_ptr, uint8_t *data_buf, uint16_t len, uint16_t offset)
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
    ota_ctx->ble.crc32 = cy_ota_update_get_calculated_crc32(ota_ctx->ble.crc32, (const uint8_t*)((uint32_t)data_buf + offset), len);
    ota_ctx->ble.file_bytes_written += chunk_info.size;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "   Downloaded 0x%lx of 0x%lx (%d%%)\n", ota_ctx->ota_storage_context.total_bytes_written, ota_ctx->ota_storage_context.total_image_size, ota_ctx->ble.percent);
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_DATA_DOWNLOAD);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_ota_update_verify(cy_ota_context_ptr ctx_ptr, uint32_t final_crc32, bool verify_crc_or_signature)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_VERIFY);

    if(verify_crc_or_signature)
    {
        /* non- secure here, check CRC */
        ota_ctx->ble.received_crc32 = final_crc32;
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "Calculated CRC : 0x%lx\n", ota_ctx->ble.crc32);
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
    }
    else
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "Skip CRC/Signature verification\n");
        result = CY_RSLT_SUCCESS;
    }

    if(result != CY_RSLT_SUCCESS)
    {
        cy_ota_set_state(ota_ctx, CY_OTA_STATE_EXITING);
    }
    else
    {
        for(uint8_t i = 1; i <= CY_OTA_IMAGE_NUMBER; i++)
        {
            ota_ctx->ota_storage_context.imgID = i;

            /* Call user image verify callback API. */
            result = ota_ctx->storage_iface.ota_file_verify(&(ota_ctx->ota_storage_context));
            if(result != CY_RSLT_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "\nStorage verify API failed: 0x%lx", result);
                result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
                cy_ota_set_state(ota_ctx, CY_OTA_STATE_EXITING);
            }
        }
    }
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_OTA_COMPLETE);

    return result;
}

cy_rslt_t cy_ota_update_image_set_pending(cy_ota_context_ptr ctx_ptr)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);

    /* Call user image verify callback API. */
    for(uint8_t i = 1; i <= CY_OTA_IMAGE_NUMBER; i++)
    {
        ota_ctx->ota_storage_context.imgID = i;
        if(ota_ctx->storage_iface.ota_file_set_boot_pending != NULL)
        {
            result = ota_ctx->storage_iface.ota_file_set_boot_pending(&(ota_ctx->ota_storage_context));
            if(result != CY_RSLT_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "\nStorage verify API failed: 0x%lx", result);
                result = CY_RSLT_OTA_ERROR_BLE_STORAGE;
                cy_ota_set_state(ota_ctx, CY_OTA_STATE_EXITING);
            }
        }
        else
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_WARNING, "ota_file_set_boot_pending not initialized!\n", result);
        }
    }
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_OTA_COMPLETE);

    return result;
}

cy_rslt_t cy_ota_update_abort(cy_ota_context_ptr ctx_ptr)
{
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s(): Set state\n", __func__);
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_AGENT_WAITING);

    return CY_RSLT_SUCCESS;
}

#endif    /* COMPONENT_OTA_BLUETOOTH */
