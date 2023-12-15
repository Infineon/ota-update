/*
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
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
 *  Cypress OTA Agent crypto for Bluetooth®
 */

#include <stdlib.h>

#include "cy_ota_ble_secure.h"
#include "cy_ota_internal.h"

#ifdef COMPONENT_OTA_BLUETOOTH_SECURE
#include "sha256.h"

extern Point ecdsa256_public_key;
#endif

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE

mbedtls_sha256_context  *bt_sha2_ctx = NULL;

/*
 * Function Name:
 * cy_ota_ble_secure_signature_init
 *
 * Function Description:
 * @brief  Get ready for checking the ecdsa 256 signature
 */
void cy_ota_ble_secure_signature_init(cy_ota_context_ptr ctx_ptr)
{
    cy_ota_context_t        *ota_ctx = (cy_ota_context_t *)ctx_ptr;

    if(bt_sha2_ctx == NULL)
    {
        bt_sha2_ctx = (mbedtls_sha256_context *)malloc(sizeof(mbedtls_sha256_context));
    }

    memset(bt_sha2_ctx, 0x00, sizeof(mbedtls_sha256_context));

    ota_ctx->ble.bt_sha2_ctx = (void *)bt_sha2_ctx;

    /* Initialize secure signature checking info */
    mbedtls_sha256_init((mbedtls_sha256_context *)ota_ctx->ble.bt_sha2_ctx);
    mbedtls_sha256_starts_ret((mbedtls_sha256_context *)ota_ctx->ble.bt_sha2_ctx, 0);
}

/*
 * Function Name:
 * app_bt_signature_check_update
 *
 * Function Description:
 * @brief  Update signature with next data chunk
 */
void cy_ota_ble_secure_signature_update(cy_ota_context_ptr ctx_ptr, uint8_t *buffer, uint32_t offset, uint32_t size_in)
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

    uint32_t total_minus_signature  = (ota_ctx->ota_storage_context.total_image_size - SIGNATURE_LEN);
    uint32_t size = size_in;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() Total bytes written: 0x%lx total minus sig: 0x%x size in: 0x%x\n", __func__, ota_ctx->ota_storage_context.total_bytes_written, total_minus_signature, size);

    /* The bytes that need to be written to FLASH are already written before this routine is called.
     * The remaining bytes in the chunk are the signature, check if we have any in this chunk.
     */
    if (ota_ctx->ota_storage_context.total_bytes_written > total_minus_signature)
    {
        /* Number of bytes after the used chunk data that are the signature */
        int i;
        uint32_t save_sig_offset;
        int32_t add_to_signature_len;

        /* compute bytes in this chunk to add to the saved signature */
        add_to_signature_len = (int32_t)ota_ctx->ota_storage_context.total_bytes_written - (int32_t)total_minus_signature;

        /* check that there are enough bytes in this chunk */
        if (add_to_signature_len > size_in)
        {
            add_to_signature_len = size_in;
        }

        /* compute offset into incoming buffer to get the data we want */
        save_sig_offset = (size_in - add_to_signature_len);

        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() saved area offset: 0x%x save_sig_offset: 0x%x add_to_signature_len : 0x%x\n", __func__, ota_ctx->ble.sig_offset, save_sig_offset, add_to_signature_len );
        /* keep for debugging        cy_ota_print_data((const char *)&buffer[save_sig_offset], add_to_signature_len); */

        for ( i = 0; i < add_to_signature_len; i++)
        {
            ota_ctx->ble.signature[ota_ctx->ble.sig_offset + i] = buffer[save_sig_offset + i];
        }
        ota_ctx->ble.sig_offset += add_to_signature_len;
        size -= add_to_signature_len;
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() SIG DATA: ota_ctx->ble.sig_offset:  0x%x\n", __func__, ota_ctx->ble.sig_offset );
        /* keep for debugging        cy_ota_print_data((const char *)ota_ctx->ble.signature, SIGNATURE_LEN); */
    }

    if (size > 0)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() call mbedtls_sha256_update_ret() 0x%lx\n", __func__, size );
        mbedtls_sha256_update_ret(ota_ctx->ble.bt_sha2_ctx, buffer, size);
    }
}

/*
 * Function Name:
 * app_bt_signature_check_verify
 *
 * Function Description:
 * @brief  Verify that the upgrade is signed properly
 */
cy_rslt_t cy_ota_ble_secure_signature_verify(cy_ota_context_ptr ctx_ptr)
{
    cy_ota_context_t            *ota_ctx = (cy_ota_context_t *)ctx_ptr;
    uint8_t                     hash[32];

    /* Finalize the signature check */
    mbedtls_sha256_finish_ret(ota_ctx->ble.bt_sha2_ctx, hash);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "VERIFY DATA\n");
    /* cy_ota_print_data((const char *)ota_ctx->ble.signature, SIGNATURE_LEN); */

    if (ota_ecdsa_verify(hash, ota_ctx->ble.signature, &ecdsa256_public_key) != 0)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "SECURE SIGNATURE CHECK FAILED\n");
        return CY_RSLT_OTA_ERROR_BLE_VERIFY;
    }
    return CY_RSLT_SUCCESS;
}
#endif
