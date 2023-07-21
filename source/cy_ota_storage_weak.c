/*
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company)
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
 * Weak functions for ota storage APIs
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cy_ota_api.h"
#include "cy_ota_internal.h"
#include "cy_ota_log.h"

/***********************************************************************
 *
 * defines & enums
 *
 * For more info on locations within slots, please see MCUBootApp
 * bootutils_misc.c, bootutils_private.h, bootutils.h
 *
 **********************************************************************/

/***********************************************************************
 *
 * Macros
 *
 **********************************************************************/
#if defined(__ICCARM__)
#define OTA_WEAK_FUNCTION        __WEAK
#define TLS_ATTR_PACKED(struct)  __packed struct
#elif defined(__GNUC__) || defined(__clang__) || defined(__CC_ARM)
#define OTA_WEAK_FUNCTION        __attribute__((weak))
#define TLS_ATTR_PACKED(struct)  struct __attribute__((packed))
#else
#define OTA_WEAK_FUNCTION        __attribute__((weak))
#define TLS_ATTR_PACKED(struct)  struct __attribute__((packed))
#endif  /* defined(__ICCARM__) */

#define UNUSED_ARG(arg)                     (void)(arg)

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/***********************************************************************
 *
 * Variables
 *
 **********************************************************************/

/***********************************************************************
 *
 * functions
 *
 **********************************************************************/
/**
 * @brief Open Storage area for download
 *
 * NOTE: Typically, this erases Secondary Slot
 *
 * @param[in]   ctx_ptr - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_OPEN_STORAGE
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_storage_open(cy_ota_context_t *ctx_ptr)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
    UNUSED_ARG(ctx_ptr);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Read from storage area
 *
 * @param[in]       ctx_ptr     - pointer to OTA agent context @ref cy_ota_context_t
 * @param[in][out]  chunk_info  - pointer to chunk information, buffer pointer used for the read
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_READ_STORAGE
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_storage_read(cy_ota_context_t *ctx_ptr, cy_ota_storage_write_info_t *chunk_info)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
    UNUSED_ARG(ctx_ptr);
	UNUSED_ARG(chunk_info);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Write to storage area
 *
 * @param[in]   ctx_ptr     - pointer to OTA agent context @ref cy_ota_context_t
 * @param[in]   chunk_info  - pointer to chunk information
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_WRITE_STORAGE
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_storage_write(cy_ota_context_t *ctx_ptr, cy_ota_storage_write_info_t *chunk_info)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
    UNUSED_ARG(ctx_ptr);
	UNUSED_ARG(chunk_info);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Close Storage area for download
 *
 * @param[in]   ctx_ptr - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_CLOSE_STORAGE
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_storage_close(cy_ota_context_t *ctx_ptr)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
    UNUSED_ARG(ctx_ptr);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Verify download signature
 *
 * @param[in]   ctx_ptr - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_storage_verify(cy_ota_context_t *ctx_ptr)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(ctx_ptr);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief App has validated the new OTA Image
 *
 * This call needs to be after reboot and MCUBoot has copied the new Application
 *      to the Primary Slot.
 *
 * @param   N/A
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_storage_validated(cy_ota_agent_mem_interface_t *flash_iface)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(flash_iface);
    return CY_RSLT_SUCCESS;
}

OTA_WEAK_FUNCTION cy_rslt_t cy_ota_write_incoming_data_block(cy_ota_context_ptr ctx_ptr, cy_ota_storage_write_info_t * const chunk_info)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
    UNUSED_ARG(ctx_ptr);
    UNUSED_ARG(chunk_info);
    return CY_RSLT_SUCCESS;
}

#ifdef FW_DATBLOCK_SEPARATE_FROM_APPLICATION

/**
 * @brief When FW is stored in a separate Data Block, get WiFi FW info
 *
 * Use this call to get the external flash WiFi info
 *
 * @param   wifi_fw_info   - ptr to cy_ota_fwdb_wifi_fw_info_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_fwdb_get_wifi_fw_info(cy_ota_fwdb_wifi_fw_info_t *wifi_fw_info)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(wifi_fw_info);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Read WiFi FW from FW DataBlock
 *
 * @param[in]   offset - offset into data
 * @param[in]   size   - amount to copy
 * @param[in]   dest   - destination buffer for the read
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_READ_STORAGE
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_fwdb_get_wifi_fw_data(uint32_t offset, uint32_t size, uint8_t *dest)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(offset);
	UNUSED_ARG(size);
	UNUSED_ARG(dest);
    return CY_RSLT_SUCCESS;
}
/**
 * @brief When FW is stored in a separate Data Block, get WiFi CLM blob info
 *
 * Use this call to get the external flash WiFi CLM blob info
 *
 * @param   clm_blob_info   - ptr to cy_ota_fwdb_clm_blob_info
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_fwdb_get_clm_blob_info(cy_ota_fwdb_clm_blob_info_t *clm_blob_info)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(clm_blob_info);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief When FW is stored in a separate Data Block, get the BT FW Patch
 *
 * Use this call to get the external flash BT FW info
 *
 * @param   bt_fw_info   - ptr to cy_ota_fwdb_bt_fw_info_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_fwdb_get_bt_fw_info(cy_ota_fwdb_bt_fw_info_t *bt_fw_info)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(bt_fw_info);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Get BT FW for transfer to BT module
 *
 * Use this call to get the external flash BT FW Patch info
 * NOTES: This allocates RAM, Expected to be < 48k
 *        The User must call cy_ota_fwdb_free_bt_fw() after use.
 *
 * @param   bt_fw   - ptr to cy_ota_fwdb_bt_fw_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_fwdb_get_bt_fw(cy_ota_fwdb_bt_fw_t *bt_fw)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(bt_fw);
    return CY_RSLT_SUCCESS;
}

/**
 * @brief Free BT FW for transfer to BT module
 *
 * Use this call to free the external flash BT FW Patch info
 * NOTES: This frees RAM, Expected to be < 48k
 *
 * @param   bt_fw   - ptr to cy_ota_fwdb_bt_fw_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
OTA_WEAK_FUNCTION cy_rslt_t cy_ota_fwdb_free_bt_fw(cy_ota_fwdb_bt_fw_t *bt_fw)
{
    /*
     * This function does nothing, weak implementation.
     * The purpose of this code is to disable compiler warnings for Non-optimized
     * builds which do not remove unused functions and require them for the
     * completeness of the linking step.
     */
	UNUSED_ARG(bt_fw);
    return CY_RSLT_SUCCESS;
}

#endif
