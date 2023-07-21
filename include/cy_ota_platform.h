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

/** @file
 *
 * The Cypress OTA API abstracts the underlying network and
 * platform support for Over-The-Air updates.
 */

/**
 * \addtogroup group_cy_ota Infineon Over The Air (OTA) API
 * \{
 * \defgroup group_ota_platform_functions OTA storage functions
 * Platform dependent functions to store/verify/install OTA update.
 *
 */

#ifndef CY_OTA_PLATFORM_H_
#define CY_OTA_PLATFORM_H_

#include "cy_ota_internal.h"

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

/**
 * \addtogroup group_ota_platform_functions
 * \{
 * OTA functions for storing, validating, and activating OTA update.
 */

/**
 * @brief Open the storage area for download.
 *
 * NOTE: Typically, this erases the secondary slot.
 *
 * @param[in]   ctx_ptr         Pointer to the OTA agent context @ref cy_ota_context_ptr.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_OPEN_STORAGE
 */
cy_rslt_t cy_ota_storage_open(cy_ota_context_ptr ctx_ptr);

/**
 * @brief Read from storage area
 *
 * @param[in]       ctx_ptr     Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]       chunk_info  Pointer to chunk information, buffer pointer used for the read
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_READ_STORAGE
 */
cy_rslt_t cy_ota_storage_read(cy_ota_context_ptr ctx_ptr, cy_ota_storage_write_info_t *chunk_info);

/**
 * @brief Write data into the storage area.
 *
 * NOTE: This writes data directly into FLASH.
 *       For writing data from a TAR archive, use cy_ota_write_incoming_data_block(),
 *       which checks for TAR archives and separates the data properly.
 *
 * @param[in]   ctx_ptr         Pointer to the OTA agent context @ref cy_ota_context_ptr.
 * @param[in]   chunk_info      Pointer to the chunk information.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_WRITE_STORAGE
 */
cy_rslt_t cy_ota_storage_write(cy_ota_context_ptr ctx_ptr, cy_ota_storage_write_info_t *chunk_info);

/**
 * @brief Determine if tar or non-tar and call correct write function
 *
 * NOTE: This function handles both TAR archive and non-TAR archive files.
 *       This function is used by Pull Mode for MQTT and HTTP
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   chunk_info              Pointer to chunk information
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_write_incoming_data_block(cy_ota_context_ptr ctx_ptr, cy_ota_storage_write_info_t * const chunk_info);


#if defined(COMPONENT_OTA_BLUETOOTH) || defined(CY_DOXYGEN)

/**
 * @brief Prepare for Bluetooth® OTA Download
 *
 * @param[in]   ota_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   bt_conn_id              Bluetooth® Connection id
 * @param[in]   bt_config_descriptor    Bluetooth® configuration (notification vs. indication )
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_GATT
 */
cy_rslt_t cy_ota_ble_download_prepare(cy_ota_context_ptr ota_ptr, uint16_t bt_conn_id, uint16_t bt_config_descriptor);

/**
 * @brief Bluetooth® OTA Download starting
 *
 * @param[in]   ota_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   p_req                   Pointer to gatt event data structure
 * @param[in]   bt_conn_id              Bluetooth® Connection id
 * @param[in]   bt_config_descriptor    Bluetooth® configuration (notification vs. indication )
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_GATT
 */
cy_rslt_t cy_ota_ble_download(cy_ota_context_ptr ota_ptr, wiced_bt_gatt_event_data_t *p_req, uint16_t bt_conn_id, uint16_t bt_config_descriptor);

/**
 * @brief Bluetooth® OTA data write
 *
 * @param[in]   ota_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   p_req                   Pointer to gatt event data structure
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_GATT
 */
cy_rslt_t cy_ota_ble_download_write(cy_ota_context_ptr ota_ptr, wiced_bt_gatt_event_data_t *p_req);

/**
 * @brief Bluetooth® OTA Verify download
 *
 * @param[in]   ota_ptr     Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   p_req       Pointer to gatt event data structure
 * @param[in]   bt_conn_id              Bluetooth® Connection id
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_GATT
 */
cy_rslt_t cy_ota_ble_download_verify(cy_ota_context_ptr ota_ptr, wiced_bt_gatt_event_data_t *p_req, uint16_t bt_conn_id);

/**
 * @brief Abort Bluetooth® OTA download
 *
 * @param[in]   ota_ptr     Pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 */
cy_rslt_t cy_ota_ble_download_abort(cy_ota_context_ptr ota_ptr);

#endif  /* defined(COMPONENT_OTA_BLUETOOTH) || defined(CY_DOXYGEN) */

/**
 * @brief Close the storage area for download.
 *
 * @param[in]   ctx_ptr - Pointer to the OTA agent context @ref cy_ota_context_ptr.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_CLOSE_STORAGE
 */
cy_rslt_t cy_ota_storage_close(cy_ota_context_ptr ctx_ptr);

/**
 * @brief Verify the download signature on the whole OTA image.
 *
 * @param[in]   ctx_ptr - Pointer to the OTA agent context @ref cy_ota_context_ptr.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_VERIFY
 */
cy_rslt_t cy_ota_storage_verify(cy_ota_context_ptr ctx_ptr);

/**
 * @brief The application has validated the new OTA image.
 *
 * This call must be after reboot and MCUboot has copied the new application
 *      to the primary slot.
 *
 * @param[in]   flash_iface - Pointer to the OTA memory operation callback context @ref cy_ota_agent_mem_interface_t.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_storage_validated(cy_ota_agent_mem_interface_t *flash_iface);

#ifdef FW_DATBLOCK_SEPARATE_FROM_APPLICATION

#define FW_DATA_block_header_info_MAGIC    "InfineonFWData  "           /* 16 bytes */
#define FW_DATA_block_header_info_VERSION  1

typedef struct  cy_ota_fw_data_block_header_s
{
    uint8_t     magic[16];          /* Magic value                                  */
    uint32_t    crc;                /* CRC of FW Data Block (not yet implemented)   */
    uint32_t    FWDB_version;       /* FW Data Block version (this structure)       */

    uint16_t    WiFi_FW_version[4]; /* WiFi FW version                              */
    uint32_t    WiFi_FW_offset;     /* Offset to start of WiFi FW                   */
    uint32_t    WiFi_FW_size;       /* Size of WiFi FW                              */

    uint32_t    CLM_blob_offset;    /* Offset to start of CLM Blob                  */
    uint32_t    CLM_blob_size;      /* Size of CLM Blob                             */

    uint8_t     BT_FW_version[128]; /* BT FW version                                */
    uint32_t    BT_FW_offset;       /* Offset to start of BT FW                     */
    uint32_t    BT_FW_size;         /* Size of BT FW                                */
} cy_ota_fw_data_block_header_t;

typedef struct cy_ota_fwdb_wifi_fw_info_s
{
    uint16_t    WiFi_FW_version[4]; /* WiFi FW version                              */
    uint32_t    WiFi_FW_addr;       /* External Flash Addr of WiFi FW               */
    uint32_t    WiFi_FW_size;       /* Size of WiFi FW                              */
} cy_ota_fwdb_wifi_fw_info_t;

typedef struct cy_ota_fwdb_clm_blob_info_s
{
    uint32_t    CLM_blob_addr;      /* External Flash Addr of WiFi FW               */
    uint32_t    CLM_blob_size;      /* Size of CLM Blob                             */
} cy_ota_fwdb_clm_blob_info_t;

typedef struct cy_ota_fwdb_bt_fw_info_s
{
    uint8_t     *BT_FW_version;     /* ptr to BT FW version                         */
    uint32_t    BT_FW_addr;         /* External Flash Addr of BT FW                 */
    uint32_t    BT_FW_size;         /* Size of BT FW                                */
} cy_ota_fwdb_bt_fw_info_t;

typedef struct cy_ota_fwdb_bt_fw_s
{
    uint8_t     *BT_FW_version;     /* ptr to BT FW version                         */
    uint8_t     *BT_FW_buffer;       /* Offset to start of BT FW                     */
    uint32_t    BT_FW_size;         /* Size of BT FW                                */
} cy_ota_fwdb_bt_fw_t;

/**
 * @brief When FW is stored in a separate Data Block, get WiFi FW info
 *
 * Use this call to get the external flash WiFi info
 *
 * @param   wifi_fw_info   - ptr to cy_ota_fwdb_wifi_fw_info
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_fwdb_get_wifi_fw_info(cy_ota_fwdb_wifi_fw_info_t *wifi_fw_info);

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
cy_rslt_t cy_ota_fwdb_get_wifi_fw_data(uint32_t offset, uint32_t size, uint8_t *dest);

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
cy_rslt_t cy_ota_fwdb_get_clm_blob_info(cy_ota_fwdb_clm_blob_info_t *clm_blob_info);


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
cy_rslt_t cy_ota_fwdb_get_bt_fw_info(cy_ota_fwdb_bt_fw_info_t *bt_fw_info);

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
cy_rslt_t cy_ota_fwdb_get_bt_fw(cy_ota_fwdb_bt_fw_t *bt_fw);

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
cy_rslt_t cy_ota_fwdb_free_bt_fw(cy_ota_fwdb_bt_fw_t *bt_fw);

#endif

/** \} group_ota_platform_functions */

#ifdef __cplusplus
    }
#endif

#endif /* CY_OTA_PLATFORM_H_ */

 /** \} group_cy_ota */
