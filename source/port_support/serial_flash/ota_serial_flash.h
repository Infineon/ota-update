/***********************************************************************************************//**
 * \file ota_serial_flash.h
 *
 * \brief
 * Provides APIs for OTA interaction with an external flash connected to the SPI or
 * QSPI interface. All functions are implemented as blocking functions.
 *
 * This is a mid-layer API to sit between OTA functions and PDL functions for serial flash access.
 *
 * We also handle cases for XIP code. This code (and lower-level code) will be placed in RAM
 * so that they can access external flash.
 *
 * This code MUST be executed from RAM.
 * 1 - Critical section is Entered (if necessary, state saved)
 * 2 - XIP is turned off (if active, state saved)
 *     - XIP Cache is cleared and turned off
 * 3 - SMIF is initialized (as needed)
 * 4 - Read/write/size inquiries are completed
 * 5 - SMIF is de-initialized
 * 6 - XIP Cache is re-enabled (if it was disabled)
 * 8 - XIP is re-enabled (if it was disabled)
 * 9 - Critical section is exited (if it was Entered)
 *
 *
 ***************************************************************************************************
 * \copyright
 *
 *
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
 **************************************************************************************************/
#pragma once

#include <stddef.h>
#include "cy_result.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "flash_map_backend.h"

#if 1 //def CY_IP_MXSMIF

#if defined(__cplusplus)
extern "C" {
#endif


/** The function or operation is not supported on the target or the memory */
#define CY_RSLT_SERIAL_FLASH_ERR_UNSUPPORTED (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_LIB_SERIAL_FLASH, 1))
/** The Serial Flash not initialized */
#define CY_RSLT_SERIAL_FLASH_ERR_NOT_INITED (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_LIB_SERIAL_FLASH, 2))
/** Parameters passed to a function are invalid */
#define CY_RSLT_SERIAL_FLASH_ERR_BAD_PARAM   (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_LIB_SERIAL_FLASH, 3))
/** A previously initiated read operation is not yet complete */
#define CY_RSLT_SERIAL_FLASH_ERR_READ_BUSY   (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_LIB_SERIAL_FLASH, 4))
/** A DMA error occurred during read transfer */
#define CY_RSLT_SERIAL_FLASH_ERR_DMA         (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_LIB_SERIAL_FLASH, 5))

/** Read abort failed. QSPI block is busy. */
#define CY_RSLT_SERIAL_FLASH_ERR_QSPI_BUSY   (CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_BOARD_LIB_SERIAL_FLASH, 6))

extern


/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

/* New API for OTA serial flash access */

/**
 * \brief Called once at startup to initialize SMIF subsystem
 * \returns CY_RSLT_SUCCESS or error condition
 */
cy_rslt_t ota_smif_initialize(void);



/**
 * \brief Returns the page size for programming of the sector to which the given
 * address belongs. Address is used only for a memory with hybrid sector size.
 * \param addr Absolute address that belongs to the sector for which size is returned.
 * \returns Page size in bytes.
 */
uint32_t ota_smif_get_prog_size(uint32_t addr);

/**
 * \brief Returns the size of the erase sector to which the given address
 * belongs. Address is used only for a memory with hybrid sector size.
 * \param addr Absolute address that belongs to the sector for which size is returned.
 * \returns Erase sector size in bytes.
 */
uint32_t ota_smif_get_erase_size(uint32_t addr);

/**
 * \brief Reads data from the serial flash memory. This is a blocking
 * function. Returns error if (addr + length) exceeds the flash size.
 *
 * \param offset Starting offset within flash area to read from
 * \param length Number of data bytes to read
 * \param buf Pointer to the buffer to store the data read from the memory
 * \returns CY_RSLT_SUCCESS if the read was successful, an error code otherwise.
 */
cy_rslt_t ota_smif_read(uint32_t offset, uint8_t *data, size_t len);

/**
 * \brief Writes the data to the serial flash memory. The program area
 * must have been erased prior to calling this API using
 * \ref cy_ota_serial_flash_qspi_erase() This is a blocking function. Returns error if
 * (addr + length) exceeds the flash size.
 * \param offset Starting offset within flash area to write to
 * \param length Number of bytes to write
 * \param buf Pointer to the buffer storing the data to be written
 * \returns CY_RSLT_SUCCESS if the write was successful, an error code
 *          otherwise.
 */
cy_rslt_t ota_smif_write(uint32_t offset, const uint8_t *data, size_t length);

/**
 * \brief Erases the serial flash memory, uses chip erase command when
 * addr = 0 and length = flash_size otherwise uses sector erase command. This is
 * a blocking function. Returns error if addr or (addr + length) is not aligned
 * to the sector size or if (addr + length) exceeds the flash size.
 * For memories with hybrid sectors, returns error if the end address
 * (addr + length) is not aligned to the size of the sector in which the end
 * address is located.
 * Call \ref ota_smif_get_size() to get the flash size and
 * call \ref ota_smif_get_erase_size() to get the size of an erase sector.
 *
 * \param offset Starting offset to begin erasing
 * \param length Number of bytes to erase
 * \returns CY_RSLT_SUCCESS if the erase was successful, an error code
 *          otherwise.
 */
cy_rslt_t ota_smif_erase(uint32_t offset, uint32_t length);

/**
 * \brief Utility function to calculate the starting address of an erase sector
 * to which the given address belongs.
 * \param addr Address in the sector for which the starting address is returned.
 * \returns Starting address of the sector
 */
uint32_t ota_smif_get_sector_start_address(uint32_t addr);


#if defined(__cplusplus)
}
#endif

#endif // CY_IP_MXSMIF
