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
 *  Cypress OTA Agent network abstraction un-tar archive
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cy_ota_api.h"
#include "cy_ota_internal.h"
#include "cy_ota_log.h"

#include "cyabs_rtos.h"

#include "untar.h"
#include "flash_map_backend.h"

/* define CY_TEST_APP_VERSION_IN_TAR to test the application version in the
 * TAR archive at start of OTA image download.
 *
 * NOTE: This requires that the user set the version number in the Makefile
 *          APP_VERSION_MAJOR
 *          APP_VERSION_MINOR
 *          APP_VERSION_BUILD
 */

/***********************************************************************
 *
 * OTA wrappers and callbacks for untar.c
 *
 **********************************************************************/

/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/

/**
 * @brief Tarball support file types recognized in components.json file
 *
 * The file type in the tarball
 */
#define CY_FILE_TYPE_SPE        "SPE"       /**< Secure Programming Environment (TFM) code type                                */
#define CY_FILE_TYPE_NSPE       "NSPE"      /**< Non-Secure Programming Environment (application) code type                    */

#ifdef FW_DATBLOCK_SEPARATE_FROM_APPLICATION
#define CY_FILE_TYPE_FWDB       "FWDB"      /**< Firmware Data Block (FWDB) type                                               */
#endif

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

/**
 * @brief Context structure for parsing the tar file
 */
static cy_untar_context_t  ota_untar_context;

/**
 * @brief Structure for handling TAR Header for MTU Sizes less than 512
 */
typedef struct update_file_header
{
    /* Temporary Buffer to handle TAR Header */
    uint8_t *buffer;

    /* Size of data in temporary Buffer */
    uint32_t buffer_size;

    /* Indicates if the TAR header check is completed */
    bool is_tar_header_checked;

} update_file_header_t;

static update_file_header_t file_header;

/***********************************************************************
 *
 * Forward declarations
 *
 **********************************************************************/

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

/**
 * @brief Write different sized chunks in 512 byte blocks
 *
 * @param fap           currently open Flash Area
 * @param offset        offset into the Flash Area to write data
 * @param source        data to use
 * @param size          amount of data in source to write
 *
 * @return  CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
static cy_untar_result_t write_data_to_flash( cy_ota_agent_mem_interface_t *flash_handle,
                                              const struct flash_area *fap, uint32_t offset,
                                              uint8_t *source, uint32_t size)
{
    /**
     * This is used if a block is < Block size to satisfy requirements
     * of flash_area_write(). "static" so it is not on the stack.
     */
    static uint8_t block_buffer[CY_FLASH_SIZEOF_ROW];
    uint32_t chunk_size = 0;

    uint32_t bytes_to_write;
    uint32_t curr_offset;
    uint8_t *curr_src;

    bytes_to_write = size;
    curr_offset = offset;
    curr_src = source;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s() write_data_to_flash() fap_off:0x%08x   off: 0x%08x  curr_off: 0x%08x\n", __func__, fap->fa_off, offset, curr_offset);

    if(flash_handle == NULL)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() Bad memory handle.\n", __func__);
        return CY_UNTAR_ERROR;
    }

    while (bytes_to_write > 0x0U)
    {
        chunk_size = bytes_to_write;
        if (chunk_size > CY_FLASH_SIZEOF_ROW)
        {
            chunk_size = CY_FLASH_SIZEOF_ROW;
        }

        /* Is the chunk_size smaller than a flash row? */
        if ( (chunk_size % CY_FLASH_SIZEOF_ROW) != 0x0U)
        {
            uint32_t row_offset = 0;
            uint32_t row_base = 0;

            row_base   = (curr_offset / CY_FLASH_SIZEOF_ROW) * CY_FLASH_SIZEOF_ROW;
            row_offset = curr_offset - row_base;

            if((row_offset + chunk_size) > CY_FLASH_SIZEOF_ROW)
            {
                chunk_size = (CY_FLASH_SIZEOF_ROW - row_offset);
            }

            /* we will read a CY_FLASH_SIZEOF_ROW byte block, write the new data into the block, then write the whole block */
            if (flash_area_read(flash_handle, fap, row_base, block_buffer, sizeof(block_buffer)) != 0)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() flash_area_read() failed\n", __func__);
                return CY_UNTAR_ERROR;
            }
            memcpy (&block_buffer[row_offset], curr_src, chunk_size);

            if (flash_area_write(flash_handle, fap, row_base, block_buffer, sizeof(block_buffer)) != 0)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%d:%s() v() failed\n", __LINE__, __func__);
                return CY_UNTAR_ERROR;
            }
        }
        else
        {
            int rc;

            rc = flash_area_write(flash_handle, fap, curr_offset, curr_src, chunk_size);
            if (rc != 0)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%d:%s() flash_area_write() failed rc:%d\n", __LINE__, __func__, rc);
                return CY_UNTAR_ERROR;
            }
        }

        curr_offset += chunk_size;
        curr_src += chunk_size;
        bytes_to_write -= chunk_size;
    }

    return CY_UNTAR_SUCCESS;
}

/**
 * @brief callback to handle tar data
 *
 * @param ctxt          untar context
 * @param file_index    index into ctxt->files for the data
 * @param buffer        data to use
 * @param file_offset   offset into the file to store data
 * @param chunk_size    amount of data in buffer to use
 * @param cb_arg        argument passed into initialization
 *
 * return   CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
static cy_untar_result_t ota_untar_write_callback(cy_untar_context_ptr ctxt,
                                   uint16_t file_index,
                                   uint8_t *buffer,
                                   uint32_t file_offset,
                                   uint32_t chunk_size,
                                   void *cb_arg)
{
    uint16_t image = 0;
    const struct flash_area *fap;
    cy_ota_context_t *ctx = (cy_ota_context_t *)cb_arg;

    if ( (ctxt == NULL) || (buffer == NULL) || (ctx == NULL))
    {
        return CY_UNTAR_ERROR;
    }

    if ( strncmp(ctxt->files[file_index].type, CY_FILE_TYPE_SPE, strlen(CY_FILE_TYPE_SPE)) == 0)
    {
        image = 1;  /* The TFM code, cm0 */
        if ( (file_offset + chunk_size) > ctxt->files[file_index].size)
        {
            chunk_size = ctxt->files[file_index].size - file_offset;
        }
    }
    else if ( strncmp(ctxt->files[file_index].type, CY_FILE_TYPE_NSPE, strlen(CY_FILE_TYPE_NSPE)) == 0)
    {
        image = 0;  /* The application code, cm4 */
        if ( (file_offset + chunk_size) > ctxt->files[file_index].size)
        {
            chunk_size = ctxt->files[file_index].size - file_offset;
        }
    }
#ifdef FW_DATBLOCK_SEPARATE_FROM_APPLICATION
    else if ( strncmp(ctxt->files[file_index].type, CY_FILE_TYPE_FWDB, strlen(CY_FILE_TYPE_FWDB)) == 0)
    {
        image = 1;  /* The Firmware Data Block */
    }
#endif
    else
    {
        /* unknown file type */
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "%d:%s Unknown File Type: >%s<\n", __LINE__, __func__, ctxt->files[file_index].type);
        return CY_UNTAR_ERROR;
    }

    if (flash_area_open(&ctx->flash_iface, FLASH_AREA_IMAGE_SECONDARY(image), &fap) != 0)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() flash_area_open(%d) failed\n", __func__, image);
        return CY_UNTAR_ERROR;
    }

    if (write_data_to_flash(&ctx->flash_iface, fap, file_offset, buffer, chunk_size) != 0)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() write_data_to_flash() failed\n", __func__);
        flash_area_close(&ctx->flash_iface, fap);
        return CY_UNTAR_ERROR;
    }

    flash_area_close(&ctx->flash_iface, fap);

    return CY_UNTAR_SUCCESS;
}

/**
 * @brief Initialization routine for handling tarball OTA file
 *
 * @param ctx_ptr               Pointer to OTA agent context
 * @param ctx_untar[in,out]     pointer to untar context to be initialized
 *
 * return   CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
static cy_untar_result_t cy_ota_untar_init_context(cy_ota_context_ptr ctx_ptr, cy_untar_context_t* ctx_untar )
{
    if (cy_untar_init( ctx_untar, ota_untar_write_callback, ctx_ptr ) == CY_RSLT_SUCCESS)
    {
        cy_ota_context_t *ctx = (cy_ota_context_t *)ctx_ptr;
        CY_OTA_CONTEXT_ASSERT(ctx);
        ctx->ota_is_tar_archive  = 1;
        return CY_UNTAR_SUCCESS;
    }
    return CY_UNTAR_ERROR;
}

#if 0 /* keep if needed later */
/**
 * @brief - set pending for the files contained in the TAR archive we just validated.
 *
 * return   CY_UNTAR_SUCCESS
 */
cy_untar_result_t cy_ota_untar_set_pending(void)
{
    uint16_t i;
    uint16_t image = 0;
    for (i = 0; i < ota_untar_context.num_files_in_json; i++ )
    {
        if ( strncmp(ota_untar_context.files[i].type, CY_FILE_TYPE_SPE, strlen(CY_FILE_TYPE_SPE)) == 0)
        {
            image = 1;  /* The TFM code, cm0 */
        }
        else if ( strncmp(ota_untar_context.files[i].type, CY_FILE_TYPE_NSPE, strlen(CY_FILE_TYPE_NSPE)) == 0)
        {
            image = 0;  /* The application code, cm4 */
        }
        else
        {
            /* unknown file type */
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%d:%s BAD FILE TYPE %d: >%s<\n", __LINE__, __func__, i, ota_untar_context.files[i].type);
            continue;
        }
        boot_set_pending(image, 0);
    }

    return CY_UNTAR_SUCCESS;

}
#endif

/**
 * @brief Determine if tar or non-tar and call correct write function
 *
 * @param[in]   ctx_ptr         Pointer to OTA agent context
 * @param[in]   chunk_info      Pointer to chunk information
 *
 * @return  CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
cy_rslt_t cy_ota_write_incoming_data_block(cy_ota_context_ptr ctx_ptr, cy_ota_storage_write_info_t * const chunk_info)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint16_t copy_offset = 0;
    cy_ota_context_t *ctx = (cy_ota_context_t *)ctx_ptr;

    if ((ctx_ptr == NULL) || (chunk_info == NULL))
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() BAD ARGUMENTS ! \n", __func__);
        return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
    }

    if (chunk_info->offset == 0UL)
    {
        file_header.is_tar_header_checked = false;
        file_header.buffer_size = 0;
    }

    if (!file_header.is_tar_header_checked)
    {
        /* we need to check some things when we receive the first 512 Bytes data */
        if ((chunk_info->offset == 0UL) && (chunk_info->size >= TAR_BLOCK_SIZE))
        {
            /*
             * Check for incoming tarball (as opposed to a single file OTA)
             */
            if (cy_is_tar_header(chunk_info->buffer, chunk_info->size) == CY_UNTAR_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%d:%s() TAR ARCHIVE\n", __LINE__, __func__);
                if (cy_ota_untar_init_context(ctx_ptr, &ota_untar_context) != CY_RSLT_SUCCESS)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_ota_untar_init_context() FAILED! \n", __func__);
                    return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
                }
            }
            else
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%d:%s() Non TAR file\n", __LINE__, __func__);
            }
            file_header.is_tar_header_checked = true;
        }
        else
        {
            if (file_header.buffer == NULL)
            {
                file_header.buffer = (uint8_t *)malloc(TAR_BLOCK_SIZE);
                if (file_header.buffer == NULL)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() malloc() for file_header Buffer FAILED! \n", __func__);
                    return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
                }
                memset(file_header.buffer, 0x00, TAR_BLOCK_SIZE);
            }

            if ((file_header.buffer_size + chunk_info->size) < TAR_BLOCK_SIZE)
            {
                memcpy((file_header.buffer + file_header.buffer_size), chunk_info->buffer, (chunk_info->size));
                file_header.buffer_size += chunk_info->size;

                return CY_RSLT_SUCCESS;
            }
            else
            {
                copy_offset = TAR_BLOCK_SIZE - file_header.buffer_size;
                memcpy((file_header.buffer + file_header.buffer_size), chunk_info->buffer, copy_offset);
                file_header.buffer_size = TAR_BLOCK_SIZE;

                chunk_info->offset = TAR_BLOCK_SIZE;
                chunk_info->size -= copy_offset;
            }

            if (cy_is_tar_header(file_header.buffer, file_header.buffer_size) == CY_UNTAR_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%d:%s() TAR ARCHIVE\n", __LINE__, __func__);
                if (cy_ota_untar_init_context(ctx_ptr, &ota_untar_context) != CY_RSLT_SUCCESS)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_ota_untar_init_context() FAILED! \n", __func__);
                    return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
                }
            }
            else
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%d:%s() Non TAR\n", __LINE__, __func__);
            }
            file_header.is_tar_header_checked = true;
        }
    }

    /* treat a tar file differently from a "normal" OTA */
    if (ctx->ota_is_tar_archive != 0)
    {
        uint32_t consumed = 0;
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%d:%s() TAR ARCHIVE\n", __LINE__, __func__);

        if (file_header.buffer_size)
        {
            cy_untar_result_t result;

            while (consumed < file_header.buffer_size)
            {
                result = cy_untar_parse(&ota_untar_context, (consumed), (file_header.buffer + consumed),
                                         (file_header.buffer_size - consumed), &consumed);
                if ((result == CY_UNTAR_ERROR) || (result == CY_UNTAR_INVALID))
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_untar_parse() FAIL consumed: %ld sz:%ld result:%ld)!\n", __func__, consumed, chunk_info->size, result);
                    return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
                }
                /* Yield for a bit */
                cy_rtos_delay_milliseconds(1);
            }
            free(file_header.buffer);
            file_header.buffer = NULL;
            file_header.buffer_size = 0;

            consumed = 0;
        }

        while (consumed < chunk_info->size)
        {
            cy_untar_result_t result;
            result = cy_untar_parse(&ota_untar_context, (chunk_info->offset + consumed), &chunk_info->buffer[consumed + copy_offset],
                                    (chunk_info->size - consumed), &consumed);
            if ((result == CY_UNTAR_ERROR) || (result == CY_UNTAR_INVALID))
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_untar_parse() FAIL consumed: %ld sz:%ld result:%ld)!\n", __func__, consumed, chunk_info->size, result);
                return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
            }

            /* Yield for a bit */
            cy_rtos_delay_milliseconds(1);
        }

        /* with the tarball we get a version - check if it is > current so we can bail early */
#ifdef CY_TEST_APP_VERSION_IN_TAR
        if (ota_untar_context.version[0] != 0)
        {
            /* example version string "<major>.<minor>.<build>" */
            uint16_t major = 0;
            uint16_t minor = 0;
            uint16_t build = 0;
            char *dot;
            major = atoi(ota_untar_context.version);
            dot = strstr(ota_untar_context.version, ".");
            if (dot != NULL)
            {
                dot++;
                minor = atoi(dot);
                dot = strstr(dot, ".");
                if (dot != NULL)
                {
                    dot++;
                    build = atoi(dot);

                    if ((major < APP_VERSION_MAJOR) ||
                          ( (major == APP_VERSION_MAJOR) &&
                            (minor < APP_VERSION_MINOR)) ||
                          ( (major == APP_VERSION_MAJOR) &&
                            (minor == APP_VERSION_MINOR) &&
                            (build <= APP_VERSION_BUILD)))
                    {
                         cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() OTA image version %d.%d.%d <= current %d.%d.%d-- bail!\r\n", __func__,
                                 major, minor, build,
                                 APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD);

                         return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
                    }
                }
            }
        }
#endif  /* CY_TEST_APP_VERSION_IN_TAR */
    }
    else
    {
        /* non-tarball OTA here, always image 0x00 */
        const struct flash_area *fap;

        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() NON-TAR file\n", __func__);
        if (flash_area_open(&ctx->flash_iface, FLASH_AREA_IMAGE_SECONDARY(0), &fap) != 0)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() flash_area_open()\n", __func__);
            return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
        }

        if (file_header.buffer_size)
        {
            result = write_data_to_flash(&ctx->flash_iface, fap, 0, file_header.buffer, file_header.buffer_size);
            free(file_header.buffer);
            file_header.buffer = NULL;
            file_header.buffer_size = 0;

            if (result != CY_RSLT_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() WRITE FAILED\n", __func__);
                return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
            }
        }

        result = write_data_to_flash(&ctx->flash_iface, fap, chunk_info->offset, (chunk_info->buffer + copy_offset), chunk_info->size);
        if (result != CY_RSLT_SUCCESS)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() WRITE FAILED\n", __func__);
            return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
        }

        flash_area_close(&ctx->flash_iface, fap);
    }

    return CY_RSLT_SUCCESS;
}
