/*
 * Copyright 2022, Cypress Semiconductor Corporation (an Infineon company)
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *  Cypress OTA Agent network abstraction for Bluetooth®
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <FreeRTOS.h>

#include "cy_ota_api.h"

#ifdef COMPONENT_OTA_BLUETOOTH

#include "cy_ota_internal.h"
#include "cycfg_gatt_db.h"
#include "cyhal_gpio.h"

#include "cybt_platform_trace.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_l2c.h"

#ifdef COMPONENT_OTA_BLUETOOTH_SECURE
extern Point ecdsa256_public_key;
#endif

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
static uint32_t crc32_Update( uint32_t prev_crc32, const uint8_t* buffer, uint16_t buffer_length )
{
    uint32_t crc32 = ~prev_crc32;
    int i;

    for ( i = 0; i < buffer_length; i++ )
    {
        int j;

        crc32 ^= *buffer++;

        for ( j = 0; j < 8; j++ )
        {
            if ( crc32 & 0x1 )
            {
                crc32 = ( crc32 >> 1 ) ^ 0xEDB88320;
            }
            else
            {
                crc32 = ( crc32 >> 1 );
            }
        }
    }

    return ~crc32;
}
#endif

/**
 * @brief Validate network parameters
 *
 * NOTE: Individual Network Connection type will test appropriate fields
 *
 * @param[in]  network_params   pointer to Network parameter structure
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_ble_validate_network_params(cy_ota_network_params_t *network_params)
{
	cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Nothing to check at the moment */

	return result;
}

/*
 * Function Name:
 * app_bt_upgrade_send_notification
 *
 * Function Description:
 * @brief  Send Notification or Indication to Host
 *
 * @param conn_id       Host connection ID
 * @param attr_handle   Handle to send to on Host
 * @param val_len       length of value pointed by p_val
 * @param p_val         Pointer to value to send
 *
 * @return wiced_bt_gatt_status_t  Bluetooth® GATT status
 */
static wiced_bt_gatt_status_t app_bt_upgrade_send_notification(uint16_t conn_id,
                                                               uint16_t bt_config_descriptor,
                                                               uint16_t attr_handle,
                                                               uint16_t val_len,
                                                               uint8_t *p_val)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    if (bt_config_descriptor & GATT_CLIENT_CONFIG_NOTIFICATION)     /* Notify & Indicate flags from HDLD_OTA_FW_UPGRADE_SERVICE_OTA_UPGRADE_CONTROL_POINT_CLIENT_CHAR_CONFIG callback */
    {
        status = wiced_bt_gatt_server_send_notification(conn_id, attr_handle, val_len, p_val, NULL);    //bt_notify_buff is not allocated, no need to keep track of it w/context
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s() Notification sent conn_id: 0x%x (%d) handle: 0x%x (%d) val_len: %d value:%d\n", __func__, conn_id, conn_id, attr_handle, attr_handle, val_len, *p_val);
    }
    else if (bt_config_descriptor & GATT_CLIENT_CONFIG_INDICATION)  /* Notify & Indicate flags from HDLD_OTA_FW_UPGRADE_SERVICE_OTA_UPGRADE_CONTROL_POINT_CLIENT_CHAR_CONFIG callback */
    {
        status = wiced_bt_gatt_server_send_indication(conn_id, attr_handle, val_len, p_val, NULL);    //bt_notify_buff is not allocated, no need to keep track of it w/context
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s() Indication sent conn_id: 0x%x (%d) handle: %d val_len: %d value:%d\n", __func__, conn_id, conn_id, attr_handle, val_len, *p_val);
    }
    if (status != WICED_BT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Notify/Indication FAILED conn_id:0x%x (%d) handle: %d val_len: %d value:%d\n", __func__, conn_id, conn_id, attr_handle, val_len, *p_val);
    }
    return status;
}

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
/*
 * Function Name:
 * cy_ota_ble_secure_signature_init
 *
 * Function Description:
 * @brief  Get ready for checking the ecdsa 256 signature
 */
static void cy_ota_ble_secure_signature_init(cy_ota_context_ptr ctx_ptr)
{
    cy_ota_context_t        *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    /* Initialize secure signature checking info */
    mbedtls_sha256_init(&ota_ctx->ble.bt_sha2_ctx);
    mbedtls_sha256_starts_ret(&ota_ctx->ble.bt_sha2_ctx, 0);
}

/*
 * Function Name:
 * app_bt_signature_check_update
 *
 * Function Description:
 * @brief  Update signature with next data chunk
 */
static void cy_ota_ble_secure_signature_update(cy_ota_context_ptr ctx_ptr, uint8_t *buffer, uint32_t offset, uint32_t size_in)
{
    cy_ota_context_t        *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    /* Update the signature check
     * Note: The last 64 bytes of the transfer are the signature itself,
     *       so we do not want to include them in checking the signature.
     *       We do save them to verify at the end of transfer.
     *
     *       Data has already been written to FLASH.
     *       We only want to update the secure signature up to the last SIGNATURE_LEN bytes.
     *       We want to save the last SIGNATURE_LEN bytes to verify.
     *
     *       They may not come in the same chunk, so we need to grab the appropriate bytes.
     */

    uint32_t total_minus_signature  = (ota_ctx->total_image_size - SIGNATURE_LEN);
    uint32_t size = size_in;

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() Total bytes written: 0x%lx total minus sig: 0x%x size in: 0x%x\n", __func__, ota_ctx->total_bytes_written, total_minus_signature, size);

    /* The bytes that need to be written to FLASH are already written before this routine is called.
     * The remaining bytes in the chunk are the signature, check if we have any in this chunk.
     */
    if (ota_ctx->total_bytes_written > total_minus_signature)
    {
        /* Number of bytes after the used chunk data that are the signature */
        int i;
        uint32_t save_sig_offset;
        int32_t add_to_signature_len;

        /* compute bytes in this chunk to add to the saved signature */
        add_to_signature_len = (int32_t)ota_ctx->total_bytes_written - (int32_t)total_minus_signature;

        /* check that there are enough bytes in this chunk */
        if (add_to_signature_len > size_in)
        {
            add_to_signature_len = size_in;
        }

        /* compute offset into incoming buffer to get the data we want */
        save_sig_offset = (size_in - add_to_signature_len);

        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() saved area offset: 0x%x save_sig_offset: 0x%x add_to_signature_len : 0x%x\n", __func__, ota_ctx->ble.sig_offset, save_sig_offset, add_to_signature_len );
//        cy_ota_print_data((const char *)&buffer[save_sig_offset], add_to_signature_len);

        for ( i = 0; i < add_to_signature_len; i++)
        {
            ota_ctx->ble.signature[ota_ctx->ble.sig_offset + i] = buffer[save_sig_offset + i];
        }
        ota_ctx->ble.sig_offset += add_to_signature_len;
        size -= add_to_signature_len;
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() SIG DATA: ota_ctx->ble.sig_offset:  0x%x\n", __func__, ota_ctx->ble.sig_offset );
//        cy_ota_print_data((const char *)ota_ctx->ble.signature, SIGNATURE_LEN);
    }

    if (size > 0)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() call mbedtls_sha256_update_ret() 0x%lx\n", __func__, size );
        mbedtls_sha256_update_ret(&ota_ctx->ble.bt_sha2_ctx, buffer, size);
    }
}

/*
 * Function Name:
 * app_bt_signature_check_verify
 *
 * Function Description:
 * @brief  Verify that the upgrade is signed properly
 */
static cy_rslt_t cy_ota_ble_secure_signature_verify(cy_ota_context_ptr ctx_ptr)
{
    cy_ota_context_t            *ota_ctx = (cy_ota_context_t *)ctx_ptr;
    uint8_t                     hash[32];

    /* Finalize the signature check */
    mbedtls_sha256_finish_ret(&ota_ctx->ble.bt_sha2_ctx, hash);

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "VERIFY DATA\n");
//    cy_ota_print_data((const char *)ota_ctx->ble.signature, SIGNATURE_LEN);

    if (ota_ecdsa_verify(hash, ota_ctx->ble.signature, &ecdsa256_public_key) != 0)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "SECURE SIGNATURE CHECK FAILED\n");
        return CY_RSLT_OTA_ERROR_BLE_VERIFY;
    }
    return CY_RSLT_SUCCESS;
}
#endif

/**
 * @brief Prepare for OTA Bluetooth® Download
 *
 * @param[in]   ota_ptr     Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   bt_conn_id  Connection id
 * @param[in]   bt_config_descriptor    configuration (notification vs. indication )
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_GATT
 */
cy_rslt_t cy_ota_ble_download_prepare(cy_ota_context_ptr ota_ptr, uint16_t bt_conn_id, uint16_t bt_config_descriptor)
{
    wiced_bt_gatt_status_t  status;
    cy_rslt_t               result;
    cy_ota_context_t        *ota_ctx = (cy_ota_context_t *)ota_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s()\n", __func__);

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    cy_ota_ble_secure_signature_init(ota_ptr);
#endif
    ota_ctx->ble.file_bytes_written = 0;

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_STORAGE_OPEN);
    result = cy_ota_storage_open(ota_ctx);      /* Call Open Storage - this erases Secondary Slot for storing downloaded OTA Image */
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "     cy_ota_storage_open() FAILED\n");
        return CY_RSLT_OTA_ERROR_BLE_GATT;
    }
    ota_ctx->ble.bt_notify_buff = CY_OTA_UPGRADE_STATUS_OK;
    if (result != CY_RSLT_SUCCESS)
    {
        ota_ctx->ble.bt_notify_buff = CY_OTA_UPGRADE_STATUS_BAD;
    }
    status = app_bt_upgrade_send_notification(bt_conn_id, bt_config_descriptor, HDLC_OTA_FW_UPGRADE_SERVICE_OTA_UPGRADE_CONTROL_POINT_VALUE, 1, &ota_ctx->ble.bt_notify_buff);
    if (status != WICED_BT_GATT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "          app_bt_upgrade_send_notification() failed: 0x%lx\n", status);
        return CY_RSLT_OTA_ERROR_BLE_GATT;
    }
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_AGENT_WAITING);
    return result;
}

/**
 * @brief Bluetooth® Download starting
 *
 * @param[in]   ota_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   p_req                   Pointer to gatt event data structure
 * @param[in]   bt_conn_id              Connection id
 * @param[in]   bt_config_descriptor    configuration (notification vs. indication )
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BADARG
 *              CY_RSLT_OTA_ERROR_WRITE_STORAGE
 */
cy_rslt_t cy_ota_ble_download(cy_ota_context_ptr ota_ptr, wiced_bt_gatt_event_data_t *p_req, uint16_t bt_conn_id, uint16_t bt_config_descriptor)
{
    wiced_bt_gatt_write_req_t   *p_write_req;
    wiced_bt_gatt_status_t          status;
    uint32_t                    total_size;

    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ota_ptr;
    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    CY_ASSERT(p_req != NULL);

    cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s()\n", __func__);
    p_write_req = &p_req->attribute_request.data.write_req;

    if (p_write_req->val_len < 4)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "CY_OTA_UPGRADE_COMMAND_DOWNLOAD len < 4\n");
        return CY_RSLT_OTA_ERROR_BLE_GATT;
    }
    total_size = (((uint32_t)p_write_req->p_val[4]) << 24) +
                 (((uint32_t)p_write_req->p_val[3]) << 16) +
                 (((uint32_t)p_write_req->p_val[2]) <<  8) +
                 (((uint32_t)p_write_req->p_val[1]) <<  0);
    cy_log_msg(CYLF_OTA, CY_LOG_INFO, "CY_OTA_UPGRADE_COMMAND_DOWNLOAD total size: 0x%lx (%ld)\n", total_size, total_size);
    ota_ctx->total_image_size = total_size;
    ota_ctx->total_bytes_written = 0;
    ota_ctx->ble.crc32 = CRC32_INITIAL_VALUE;
    ota_ctx->ble.percent = 0;
    /* Send notification that we are prepared for the data */
    ota_ctx->ble.bt_notify_buff = CY_OTA_UPGRADE_STATUS_OK;
    status = app_bt_upgrade_send_notification(bt_conn_id, bt_config_descriptor, HDLC_OTA_FW_UPGRADE_SERVICE_OTA_UPGRADE_CONTROL_POINT_VALUE, 1, &ota_ctx->ble.bt_notify_buff);
    if (status != WICED_BT_GATT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "          app_bt_upgrade_send_notification() failed: 0x%lx\n", status);
        return CY_RSLT_OTA_ERROR_BLE_GATT;
    }

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_START_UPDATE);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Bluetooth® OTA data write
 *
 * @param[in]   ota_ptr     - pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   p_req       - ptr to gatt event data structure
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BADARG
 *              CY_RSLT_OTA_ERROR_WRITE_STORAGE
 */
cy_rslt_t cy_ota_ble_download_write(cy_ota_context_ptr ota_ptr, wiced_bt_gatt_event_data_t *p_req)
{
    wiced_bt_gatt_write_req_t       *p_write_req;
    cy_ota_storage_write_info_t     chunk_info;
    cy_rslt_t                       result;
    uint32_t                        full_write_size;
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ota_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    CY_ASSERT(p_req != NULL);

    cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s()\n", __func__);
    p_write_req = &p_req->attribute_request.data.write_req;

    /* prepare to call ota library write routine */
    memset(&chunk_info, 0x00, sizeof(chunk_info));
    chunk_info.buffer = p_write_req->p_val;
    chunk_info.offset = p_write_req->offset;
    chunk_info.size   = p_write_req->val_len;
    full_write_size   = chunk_info.size;

    /* Don't require App to keep track of offset into the Slot
     * We also handle the offset handed in, in case we are pointing into a buffer
     * rather than the start of the buffer
     */
    chunk_info.buffer += chunk_info.offset;   /* Handle offset into the buffer given us */
    chunk_info.offset = ota_ctx->last_offset;      /* Need to set the offset into Secondary Slot properly for the write */

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_STORAGE_WRITE);

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
	if (chunk_info.offset + chunk_info.size > FLASH_AREA_IMG_1_SECONDARY_SIZE)
	{
		chunk_info.size = FLASH_AREA_IMG_1_SECONDARY_SIZE - chunk_info.offset;
	}
#endif

	if (chunk_info.size > 0)
	{
		result = cy_ota_write_incoming_data_block(ota_ptr, &chunk_info);
		if (result != CY_RSLT_SUCCESS)
		{
			cy_rtos_setbits_event(&ota_ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_FAIL, 0);

			cy_log_msg(CYLF_OTA, CY_LOG_ERR, "     cy_ota_write_incoming_data_block() FAILED : 0x%lx \n", result);
			return CY_RSLT_OTA_ERROR_BLE_GATT;
		}
	}

    /* Make sure the chunk info size is correct in case we needed to strip
     * out the trailing signature for secure BLE.
     */

    chunk_info.size = full_write_size;

    /* update the stats */
    ota_ctx->total_bytes_written   += chunk_info.size;      /* Total bytes written to flash         */
    ota_ctx->last_size              = chunk_info.size;      /* last data size received              */
    ota_ctx->last_offset           += chunk_info.size;      /* Keep track of offset into the Slot   */
    if (ota_ctx->total_image_size > 0)
    {
        ota_ctx->ble.percent = (100 * ota_ctx->total_bytes_written) / ota_ctx->total_image_size;
    }

    /* update crc or secure signature as we pull in the data */
#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    cy_ota_ble_secure_signature_update(ota_ptr, p_write_req->p_val, p_write_req->offset, p_write_req->val_len);
#else
    ota_ctx->ble.crc32 = crc32_Update(ota_ctx->ble.crc32, p_write_req->p_val + p_write_req->offset, p_write_req->val_len);
#endif
    ota_ctx->ble.file_bytes_written += chunk_info.size;

    cy_log_msg(CYLF_OTA, CY_LOG_NOTICE, "   Downloaded 0x%lx of 0x%lx (%d%%)\n", ota_ctx->total_bytes_written, ota_ctx->total_image_size, ota_ctx->ble.percent);
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_DATA_DOWNLOAD);
    return CY_RSLT_SUCCESS;

}

/**
 * @brief Bluetooth® OTA Verify download
 *
 * @param[in]   ota_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   p_req                   Pointer to gatt event data structure
 * @param[in]   bt_conn_id              Connection id
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_VERIFY
 */
cy_rslt_t cy_ota_ble_download_verify(cy_ota_context_ptr ota_ptr, wiced_bt_gatt_event_data_t *p_req, uint16_t bt_conn_id)
{
    wiced_bt_gatt_status_t          status;
    cy_rslt_t                       result = CY_RSLT_SUCCESS;
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ota_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);

    cy_ota_set_state(ota_ctx, CY_OTA_STATE_VERIFY);

#ifndef  COMPONENT_OTA_BLUETOOTH_SECURE
    wiced_bt_gatt_write_req_t       *p_write_req;
    CY_ASSERT(p_req != NULL);
    p_write_req = &p_req->attribute_request.data.write_req;
#else
    (void)p_req;
#endif
    /* Presume positive outcome for notification */
    ota_ctx->ble.bt_notify_buff = CY_OTA_UPGRADE_STATUS_OK;

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    result = cy_ota_ble_secure_signature_verify(ota_ptr);
    if (result == CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_NOTICE, "     Bluetooth(r) Secure Signature Verification Succeeded!\n");
    }

#else
    /* non- secure here, check CRC */
    if (p_write_req->val_len == 5)
    {
        ota_ctx->ble.received_crc32 = ( ( (uint32_t)p_write_req->p_val[1]) <<  0) +
                                    ( ( (uint32_t)p_write_req->p_val[2]) <<  8) +
                                    ( ( (uint32_t)p_write_req->p_val[3]) << 16) +
                                    ( ( (uint32_t)p_write_req->p_val[4]) << 24);

        if (ota_ctx->ble.crc32 != ota_ctx->ble.received_crc32)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_INFO, "     check CRC FAILED 0x%lx != 0x%lx\n", ota_ctx->ble.crc32, ota_ctx->ble.received_crc32);
            result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
        }
        else
        {
            cy_log_msg(CYLF_OTA, CY_LOG_NOTICE, "     Bluetooth(r) CRC Verification Succeeded!\n");
            result = CY_RSLT_SUCCESS;
        }
    }
    else
    {
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "     check CRC - len from Host !=5 %d\n", p_write_req->val_len);
        result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
    }
#endif

    if (result == CY_RSLT_SUCCESS)
    {
        /* set OTA library status to verified */
        /* mark the storage verified so that we will do update on next reboot */
        result = cy_ota_storage_verify(ota_ctx);
        if (result != CY_RSLT_SUCCESS)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_ERR, "          cy_ota_storage_verify() failed: 0x%lx\n", result);
            result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
        }
    }
    else
    {
        ota_ctx->ble.bt_notify_buff = CY_OTA_UPGRADE_STATUS_BAD;
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "     call wiced_bt_gatt_send_indication(%d) !\n", ota_ctx->ble.bt_notify_buff);
    }

    /* Send indication that we are done & verified or not */
    status = wiced_bt_gatt_server_send_indication(bt_conn_id, HDLC_OTA_FW_UPGRADE_SERVICE_OTA_UPGRADE_CONTROL_POINT_VALUE,
                                                    1, &ota_ctx->ble.bt_notify_buff, NULL);    //bt_notify_buff is not allocated, no need to keep track of it w/context
    if (status != WICED_BT_GATT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "          app_bt_upgrade_send_indication() failed: 0x%lx\n", status);
        result = CY_RSLT_OTA_ERROR_BLE_VERIFY;
        cy_ota_set_state(ota_ctx, CY_OTA_STATE_EXITING);
    }
    else
    {
        cy_ota_set_state(ota_ctx, CY_OTA_STATE_OTA_COMPLETE);
    }
    return result;
}

/**
 * @brief Abort Bluetooth® OTA download
 *
 * @param[in]   ota_ptr     - pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_XXXX
 */
cy_rslt_t cy_ota_ble_download_abort(cy_ota_context_ptr ota_ptr)
{
    cy_ota_context_t *ota_ctx = (cy_ota_context_t *)ota_ptr;

    CY_OTA_CONTEXT_ASSERT(ota_ctx);
    cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s(): Set state\n", __func__);
    cy_ota_set_state(ota_ctx, CY_OTA_STATE_AGENT_WAITING);

    return CY_RSLT_SUCCESS;
}


#endif 	/* COMPONENT_OTA_BLUETOOTH */
