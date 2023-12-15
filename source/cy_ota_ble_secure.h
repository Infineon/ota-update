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

#ifndef CY_OTA_BLE_SECURE_H__
#define CY_OTA_BLE_SECURE_H__ 1

#include "cy_ota_api.h"

/**
 * @brief Get ready for checking the ecdsa 256 signature
 *
 * @param[in]   ctx_ptr              - Pointer to the OTA Agent context.
 *
 */
void cy_ota_ble_secure_signature_init(cy_ota_context_ptr ctx_ptr);

/**
 * @brief Update signature with next data chunk
 *
 * @param[in]   ctx_ptr              - Pointer to OTA agent context.
 * @param[in]   buffer               - Pointer to data chunk.
 * @param[in]   offset               - Offset in data buffer.
 * @param[in]   size_in              - Size of data.
 */
void cy_ota_ble_secure_signature_update(cy_ota_context_ptr ctx_ptr, uint8_t *buffer, uint32_t offset, uint32_t size_in);

/**
 * @brief  Verify that the upgrade is signed properly
 *
 * @param[in]   ctx_ptr             - Pointer to OTA agent context.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_BLE_VERIFY
 */
cy_rslt_t cy_ota_ble_secure_signature_verify(cy_ota_context_ptr ctx_ptr);

#endif /* CY_OTA_BLE_SECURE_H__ */
