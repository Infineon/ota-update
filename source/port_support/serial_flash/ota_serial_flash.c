/***********************************************************************************************//**
 * \file ota_serial_flash.c
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
 * init - SMIF is initialized once
 *
 * This is the sequence BEFORE each SMIF access:
 * 1 - Critical section is Entered (if necessary, state saved)
 * 2 - XIP is turned off and cleared
 *
 * - Read/write/size inquiries are completed
 *
 * This is the sequence AFTER each SMIF access:
 * 1 - XIP is re-enabled (if it was disabled)
 * 2 - Critical section is exited (if it was Entered)
 *
 * NOTES: This code assumes that system starts in XIP mode and returns in XIP mode.
 *          When using MCUBootApp and executing from external flash, we assume system is set for XIP mode.
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

#include <stddef.h>
#include "cy_result.h"
#include "cy_pdl.h"
#include "cyhal.h"
#include "flash_map_backend.h"
#include "sysflash.h"
#include "cy_log.h"

#include "ota_serial_flash.h"

#ifdef CY_IP_MXSMIF

#if defined(__cplusplus)
extern "C" {
#endif

/**********************************************************************************************************************************
 * local defines
 **********************************************************************************************************************************/

/* UN-comment to test the write functionality */
//#define READBACK_SMIF_WRITE_TEST

// SMIF slot from which the memory configuration is picked up - fixed to 0 as
// the driver supports only one device
#define MEM_SLOT                                    (0u)

/* Set it high enough for the sector erase operation to complete */
#define MEMORY_BUSY_CHECK_RETRIES   (750ul)

#define _CYHAL_QSPI_DESELECT_DELAY (7UL)

/* Masks used for checking the flag bits */
#define FLAG_HAL_INIT_DONE                     (0x01lu << 0)           /* cyhal_qspi_init() succeeded */

#define IS_FLAG_SET(mask)                           (status_flags & (mask))
#define SET_FLAG(mask)                              (status_flags |= (mask))
#define CLEAR_FLAG(mask)                            (status_flags &= ~(mask))

/* QSPI bus frequency set to 50 Mhz */
#define QSPI_BUS_FREQUENCY_HZ   (50000000lu)

#define TIMEOUT_1_MS        1000lu

#ifdef CY_XIP_SMIF_MODE_CHANGE

/*
 * IMPORTANT NOTE. Do not add calls to non-RAM resident routines
 * between TURN_OFF_XIP and TURN_ON_XIP calls or you will break
 * XIP environments.
 */

#define PRE_SMIF_ACCESS_TURN_OFF_XIP \
                    uint32_t interruptState;                            \
                    interruptState = Cy_SysLib_EnterCriticalSection();  \
                    while(Cy_SMIF_BusyCheck(SMIF0));    \
                    (void)Cy_SMIF_SetMode(SMIF0, CY_SMIF_NORMAL);

#define POST_SMIF_ACCESS_TURN_ON_XIP \
                    while(Cy_SMIF_BusyCheck(SMIF0));    \
                    (void)Cy_SMIF_SetMode(SMIF0, CY_SMIF_MEMORY);   \
                    Cy_SysLib_ExitCriticalSection(interruptState);


#else
#define PRE_SMIF_ACCESS_TURN_OFF_XIP
#define POST_SMIF_ACCESS_TURN_ON_XIP
#endif

/**********************************************************************************************************************************
 * local variables & data
 **********************************************************************************************************************************/

static cy_stc_smif_context_t ota_QSPI_context;
static volatile uint32_t status_flags;

/* Default QSPI configuration */
const cy_stc_smif_config_t ota_SMIF_config =
{
    .mode = (uint32_t)CY_SMIF_NORMAL,
    .deselectDelay = _CYHAL_QSPI_DESELECT_DELAY,
#if (CY_IP_MXSMIF_VERSION >= 2)
    .rxClockSel = (uint32_t)CY_SMIF_SEL_INVERTED_FEEDBACK_CLK,
#else
    .rxClockSel = (uint32_t)CY_SMIF_SEL_INV_INTERNAL_CLK,
#endif
    .blockEvent = (uint32_t)CY_SMIF_BUS_ERROR,
};

extern const cy_stc_smif_mem_config_t* const smifMemConfigs[];
extern const cy_stc_smif_block_config_t smifBlockConfig;

#ifdef READBACK_SMIF_WRITE_TEST
/* Used for testing the write functionality */
static uint8_t read_back_test[1024];
#endif

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

/* New API for OTA serial flash access */

/**********************************************************************************************************************************
 * Internal Functions
 **********************************************************************************************************************************/

/*******************************************************************************
* Function Name: IsMemoryReady
****************************************************************************//**
*
* Polls the memory device to check whether it is ready to accept new commands or
* not until either it is ready or the retries have exceeded the limit.
*
* \param memConfig
* memory device configuration
*
* \return Status of the operation.
* CY_SMIF_SUCCESS          - Memory is ready to accept new commands.
* CY_SMIF_EXCEED_TIMEOUT - Memory is busy.
*
*******************************************************************************/
cy_en_smif_status_t IsMemoryReady(cy_stc_smif_mem_config_t const *memConfig)
{
    uint32_t retries = 0;
    bool isBusy;

    do
    {
        isBusy = Cy_SMIF_Memslot_IsBusy(SMIF0, (cy_stc_smif_mem_config_t* )memConfig, &ota_QSPI_context);
        Cy_SysLib_Delay(5);
        retries++;
    }while(isBusy && (retries < MEMORY_BUSY_CHECK_RETRIES));

    return (isBusy ? CY_SMIF_EXCEED_TIMEOUT : CY_SMIF_SUCCESS);
}
/*******************************************************************************
* Function Name: IsQuadEnabled
****************************************************************************//**
*
* Checks whether QE (Quad Enable) bit is set or not in the configuration
* register of the memory.
*
* \param memConfig
* Memory device configuration
*
* \param isQuadEnabled
* This parameter is updated to indicate whether Quad mode is enabled (true) or
* not (false). The value is valid only when the function returns
* CY_SMIF_SUCCESS.
*
* \return Status of the operation. See cy_en_smif_status_t.
*
*******************************************************************************/
cy_en_smif_status_t IsQuadEnabled(cy_stc_smif_mem_config_t const *memConfig, bool *isQuadEnabled)
{
    cy_en_smif_status_t status;
    uint8_t readStatus = 0;
    uint32_t statusCmd = memConfig->deviceCfg->readStsRegQeCmd->command;
    uint8_t maskQE = (uint8_t) memConfig->deviceCfg->stsRegQuadEnableMask;

    status = Cy_SMIF_Memslot_CmdReadSts(SMIF0, memConfig, &readStatus, statusCmd, &ota_QSPI_context);

    *isQuadEnabled = false;
    if(CY_SMIF_SUCCESS == status)
    {
        /* Check whether Quad mode is already enabled or not */
        *isQuadEnabled = (maskQE == (readStatus & maskQE));
    }

    return status;
}


/*******************************************************************************
* Function Name: EnableQuadMode
****************************************************************************//**
*
* This function sets the QE (QUAD Enable) bit in the external memory
* configuration register to enable Quad SPI mode.
*
* \param memConfig
* Memory device configuration
*
* \return Status of the operation. See cy_en_smif_status_t.
*
*******************************************************************************/
cy_en_smif_status_t EnableQuadMode(cy_stc_smif_mem_config_t const *memConfig)
{
    cy_en_smif_status_t status;

    /* Send Write Enable to external memory */
    status = Cy_SMIF_Memslot_CmdWriteEnable(SMIF0, smifMemConfigs[0], &ota_QSPI_context);

    if(CY_SMIF_SUCCESS == status)
    {
        status = Cy_SMIF_Memslot_QuadEnable(SMIF0, (cy_stc_smif_mem_config_t* )memConfig, &ota_QSPI_context);

        if(CY_SMIF_SUCCESS == status)
        {
            /* Poll memory for the completion of operation */
            status = IsMemoryReady(memConfig);
        }
    }

    return status;
}


/**********************************************************************************************************************************
 * External Functions
 **********************************************************************************************************************************/

#if !defined(CYW20829A0LKML)
#include <cycfg_pins.h>
#endif

cy_rslt_t ota_smif_initialize(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_rslt_t smif_status = CY_SMIF_BAD_PARAM;    /* Does not return error if SMIF Quad fails */
    bool QE_status = false;

    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() \n", __func__);

#if !defined(CYW20829A0LKML)
    /* Disable SMIF Cache - it is not working in CY8CPROTO-062S3-4343W context */
    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() call Cy_SMIF_CacheDisable()\n", __func__);
    Cy_SMIF_CacheDisable(SMIF0, CY_SMIF_CACHE_BOTH);
    while(Cy_SMIF_BusyCheck(SMIF0));
#endif

    /* pre-access to SMIF */
    PRE_SMIF_ACCESS_TURN_OFF_XIP;

#if defined(CYW20829A0LKML)
    /* SMIF is already initialized for 20829 so we are only initializing the
     * SMIF base address and the context variables.
     */
    smif_status = Cy_SMIF_Init(SMIF0, &ota_SMIF_config, TIMEOUT_1_MS, &ota_QSPI_context);
    if (smif_status != CY_SMIF_SUCCESS)
    {
        result = smif_status;
        goto _bail;
    }
#endif

    /* Set up SMIF */
    Cy_SMIF_SetDataSelect(SMIF0, smifMemConfigs[0]->slaveSelect, smifMemConfigs[0]->dataSelect);
    Cy_SMIF_Enable(SMIF0, &ota_QSPI_context);

    /* Map memory device to memory map */
    smif_status = Cy_SMIF_Memslot_Init(SMIF0, &smifBlockConfig, &ota_QSPI_context);
    if (smif_status != CY_SMIF_SUCCESS)
    {
        // CY_HALT();
        result = smif_status;
        goto _bail;
    }

#if defined(CYW20829A0LKML)
    /* Even after SFDP enumeration QE command is not initialized */
    /* So, it should be 1.0 device */
    if ((smifMemConfigs[0]->deviceCfg->readStsRegQeCmd->command == 0) ||                        /* 0 - if configurator generated code */
        (smifMemConfigs[0]->deviceCfg->readStsRegQeCmd->command == CY_SMIF_NO_COMMAND_OR_MODE)) /* 0xFF's if SFDP enumerated          */
    {
        smif_status = Cy_SMIF_MemInitSfdpMode(SMIF0,
                                              smifMemConfigs[0],
                                              CY_SMIF_WIDTH_QUAD,
                                              CY_SMIF_SFDP_QER_1,
                                              &ota_QSPI_context);
        if (smif_status != CY_SMIF_SUCCESS)
        {
            result = smif_status;
            goto _bail;
        }
    }
#else /* NON - CYW20829A0LKML */
#if !defined(CY_RUN_CODE_FROM_XIP) && (OTA_USE_EXTERNAL_FLASH)
    {
        /* Choose SMIF slot number (slave select).
         * Acceptable values are:
         * 0 - SMIF disabled (no external memory);
         * 1, 2, 3 or 4 - slave select line memory module is connected to.
         */
#define SMIF_ID         (1U) /* Assume SlaveSelect_0 is used for External Memory */
#include "flash_qspi.h"
        cy_en_smif_status_t qspi_status = qspi_init_sfdp(SMIF_ID);

        if (CY_SMIF_SUCCESS == qspi_status) {
            result = CY_RSLT_SUCCESS;
        } else {
            result = CY_RSLT_TYPE_ERROR;
        }
    }
#endif /* ! CY_RUN_CODE_FROM_XIP */
#endif /* CYW20829A0LKML */

    smif_status = IsQuadEnabled(smifMemConfigs[0], &QE_status);
    if (smif_status != CY_RSLT_SUCCESS)
    {
    }

    /* If not enabled, enable quad mode */
    if(!QE_status)
    {
        /* Enable Quad mode */
        smif_status = EnableQuadMode(smifMemConfigs[0]);
        if (smif_status != CY_RSLT_SUCCESS)
        {
            // CY_HALT();
        }
    }

    SET_FLAG(FLAG_HAL_INIT_DONE);

  _bail:
    /* post-access to SMIF */
    POST_SMIF_ACCESS_TURN_ON_XIP;

    return result;
}


uint32_t ota_smif_get_memory_size(void)
{
    uint32_t size = 0;

    /* pre-access to SMIF is not needed, as we are just reading data from RAM */
    if (IS_FLAG_SET(FLAG_HAL_INIT_DONE))
    {
        if (SMIF0 != NULL)
        {
            size = smifBlockConfig.memConfig[MEM_SLOT]->deviceCfg->memSize;
        }
    }
    /* post-access to SMIF  is not needed, as we are just reading data from RAM */

    return size;
}

uint32_t ota_smif_get_prog_size(uint32_t addr)
{
    uint32_t    program_size = 0;
    (void)addr; /* Hybrid parts not yet supported */

    /* pre-access to SMIF is not needed, as we are just reading data from RAM */
    if (IS_FLAG_SET(FLAG_HAL_INIT_DONE))
    {
        if (SMIF0 != NULL)
        {
            program_size = smifBlockConfig.memConfig[MEM_SLOT]->deviceCfg->programSize;
        }
    }
    /* post-access to SMIF  is not needed, as we are just reading data from RAM */

    return program_size;
}

uint32_t ota_smif_get_erase_size(uint32_t address)
{
    uint32_t                            erase_sector_size = 0;
    cy_stc_smif_hybrid_region_info_t*   hybrid_info = NULL;
    cy_en_smif_status_t                 smif_status;

    if (address >= CY_SMIF_BASE_MEM_OFFSET)
    {
        address -= CY_SMIF_BASE_MEM_OFFSET;
    }

    /* pre-access to SMIF is not needed, as we are just reading data from RAM */
    if (IS_FLAG_SET(FLAG_HAL_INIT_DONE))
    {
        /* Cy_SMIF_MemLocateHybridRegion() does not access the external flash, just data tables from RAM  */
        smif_status = Cy_SMIF_MemLocateHybridRegion(smifBlockConfig.memConfig[MEM_SLOT], &hybrid_info, address);

        if (CY_SMIF_SUCCESS != smif_status)
        {
            erase_sector_size = (size_t)smifBlockConfig.memConfig[MEM_SLOT]->deviceCfg->eraseSize;
        }
        else
        {
            erase_sector_size = (size_t)hybrid_info->eraseSize;
        }
    }
    /* post-access to SMIF is not needed, as we are just reading data from RAM */

    return erase_sector_size;
}

cy_rslt_t ota_smif_read(uint32_t offset, uint8_t *buffer, size_t length)
{
    cy_rslt_t cy_smif_result = CY_RSLT_SERIAL_FLASH_ERR_NOT_INITED;

    if (offset >= CY_SMIF_BASE_MEM_OFFSET)
    {
        offset -= CY_SMIF_BASE_MEM_OFFSET;
    }

    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() READ %d bytes from : 0x%08lx\n", __func__, length, offset);

    if (IS_FLAG_SET(FLAG_HAL_INIT_DONE))
    {
        /* pre-access to SMIF */
        PRE_SMIF_ACCESS_TURN_OFF_XIP;

        cy_smif_result = (cy_rslt_t)Cy_SMIF_MemRead(SMIF0, smifBlockConfig.memConfig[MEM_SLOT],
                                                    offset, buffer, length, &ota_QSPI_context);
        /* post-access to SMIF */
        POST_SMIF_ACCESS_TURN_ON_XIP;
    }

    if (cy_smif_result != CY_SMIF_SUCCESS)
    {
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() FAILED cy_smif_result: 0x%x\n", __func__, cy_smif_result);
    }

    return (cy_smif_result == CY_SMIF_SUCCESS) ? CY_RSLT_SUCCESS : CY_RSLT_TYPE_ERROR;
}

cy_rslt_t ota_smif_write(uint32_t offset, const uint8_t *buffer, size_t length)
{
    cy_en_smif_status_t cy_smif_result = CY_RSLT_SERIAL_FLASH_ERR_NOT_INITED;

    if (offset >= CY_SMIF_BASE_MEM_OFFSET)
    {
        offset -= CY_SMIF_BASE_MEM_OFFSET;
    }

    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() WRITE length 0x%08lx bytes to offset: 0x%08lx \n", __func__, length, offset);

    if (IS_FLAG_SET(FLAG_HAL_INIT_DONE))
    {
        /* pre-access to SMIF */
        PRE_SMIF_ACCESS_TURN_OFF_XIP;

        cy_smif_result = Cy_SMIF_MemWrite(SMIF0, smifBlockConfig.memConfig[MEM_SLOT],
                offset, buffer, length, &ota_QSPI_context);

        /* post-access to SMIF */
        POST_SMIF_ACCESS_TURN_ON_XIP;
    }

#ifdef READBACK_SMIF_WRITE_TEST
    if (cy_smif_result == CY_SMIF_SUCCESS)
    {
        if (ota_smif_read(offset, read_back_test, ((16 < length) ? 16 : length)) == CY_RSLT_SUCCESS)
        {
            int i;
            for (i=0; (i<16 && i<length); i++)
            {
                if(buffer[i] != read_back_test[i])
                {
                    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "MISMATCH: i:%d 0x%02x  != 0x%02x\n", i, buffer[i], read_back_test[i]);
                    cy_smif_result  = -1;
                }
            }
        }
    }
#endif

    if (cy_smif_result != CY_SMIF_SUCCESS)
    {
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() FAILED cy_smif_result: 0x%lx\n", __func__, cy_smif_result);
    }

    return (cy_smif_result == CY_SMIF_SUCCESS) ? CY_RSLT_SUCCESS : CY_RSLT_TYPE_ERROR;
}

cy_rslt_t ota_smif_erase(uint32_t offset, uint32_t length)
{
    cy_en_smif_status_t cy_smif_result = CY_PDL_STATUS_ERROR;
    uint32_t interruptState;
    (void)interruptState;

    if (offset >= CY_SMIF_BASE_MEM_OFFSET)
    {
        offset -= CY_SMIF_BASE_MEM_OFFSET;
    }

    if (IS_FLAG_SET(FLAG_HAL_INIT_DONE))
    {
        /* pre-access to SMIF */
        PRE_SMIF_ACCESS_TURN_OFF_XIP;

        // If the erase is for the entire chip, use chip erase command
        if ((offset == 0u) && (length == ota_smif_get_memory_size()))
        {
            cy_smif_result = Cy_SMIF_MemEraseChip(SMIF0,
                                                smifBlockConfig.memConfig[MEM_SLOT],
                                                &ota_QSPI_context);
        }
        else
        {
            // Cy_SMIF_MemEraseSector() returns error if (addr + length) > total flash size or if
            // addr is not aligned to erase sector size or if (addr + length) is not aligned to
            // erase sector size.
            /* Make sure the base offset is correct */
            uint32_t erase_size;
            uint32_t diff;
            erase_size = ota_smif_get_erase_size(offset);
            diff = offset & (erase_size - 1);
            offset -= diff;
            length += diff;
            /* Make sure the length is correct */
            length = (length + (erase_size - 1)) & ~(erase_size - 1);
            cy_smif_result = Cy_SMIF_MemEraseSector(SMIF0,
                                                  smifBlockConfig.memConfig[MEM_SLOT],
                                                  offset, length, &ota_QSPI_context);
        }

        /* post-access to SMIF */
        POST_SMIF_ACCESS_TURN_ON_XIP;
    }
    if (cy_smif_result != CY_SMIF_SUCCESS)
    {
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() FAILED cy_smif_result: 0x%lx\n", __func__, cy_smif_result);
    }

    return (cy_smif_result == CY_SMIF_SUCCESS) ? CY_RSLT_SUCCESS : CY_RSLT_TYPE_ERROR;
}

uint32_t ota_smif_get_sector_start_address(uint32_t addr)
{
    return (addr & ~(ota_smif_get_erase_size(addr) - 1));
}


#if defined(__cplusplus)
}
#endif

#endif // CY_IP_MXSMIF
