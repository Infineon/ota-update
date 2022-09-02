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
 *
 * Cypress Un-tar utility.
 *
 */

#ifndef UNTAR_H__
#define UNTAR_H__   1


#include <stdint.h>
#include "cy_result.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @ Unix Standard Tar header format
 *
 * The Open Group Base Specifications Issue 7, 2018 edition
 * IEEE Std 1003.1-2017 (Revision of IEEE Std 1003.1-2008)
 * Copyright � 2001-2018 IEEE and The Open Group
 *
 * ustar Interchange Format
 * A ustar archive tape or file shall contain a series of logical records.
 * Each logical record shall be a fixed-size logical record of 512 octets (see below).
 * Although this format may be thought of as being stored on 9-track industry-standard
 * 12.7 mm (0.5 in) magnetic tape, other types of transportable media are not excluded.
 * Each file archived shall be represented by a header logical record that describes
 * the file, followed by zero or more logical records that give the contents of the file.
 * At the end of the archive file there shall be two 512-octet logical records filled with
 * binary zeros, interpreted as an end-of-archive indicator.
 * The logical records may be grouped for physical I/O operations, as described under the
 * -b blocksize and -x ustar options. Each group of logical records may be written with a
 * single operation equivalent to the write() function. On magnetic tape, the result of
 * this write shall be a single tape physical block. The last physical block shall always
 * be the full size, so logical records after the two zero logical records may contain undefined data.
 *
 */

#define TAR_BLOCK_SIZE  512

/* General definitions. */
#define TNAMELEN    100         /**< Name field length. */
#define TMAGIC      "ustar "    /**< ustar plus null byte. */
#define TMAGLEN     6           /**< Length of the above. */
#define TVERSION    "00"        /**< 00 without a null byte. */
#define TVERSLEN    2           /**< Length of the above. */

/* Typeflag field definitions. */
#define REGTYPE     '0'         /**< Regular file. */
#define AREGTYPE    '\0'        /**< Regular file. */
#define LNKTYPE     '1'         /**< Link. */
#define SYMTYPE     '2'         /**< Symbolic link. */
#define CHRTYPE     '3'         /**< Character special. */
#define BLKTYPE     '4'         /**< Block special. */
#define DIRTYPE     '5'         /**< Directory. */
#define FIFOTYPE    '6'         /**< FIFO special. */
#define CONTTYPE    '7'         /**< Reserved. */

/* Mode field bit definitions (octal). */
#define TSUID       04000       /**< Set UID on execution. */
#define TSGID       02000       /**< Set GID on execution. */
#define TSVTX       01000       /**< On directories, restricted deletion flag. */
#define TUREAD      00400       /**< Read by owner. */
#define TUWRITE     00200       /**< Write by owner. */
#define TUEXEC      00100       /**< Execute/search by owner. */
#define TGREAD      00040       /**< Read by group. */
#define TGWRITE     00020       /**< Write by group. */
#define TGEXEC      00010       /**< Execute/search by group. */
#define TOREAD      00004       /**< Read by other. */
#define TOWRITE     00002       /**< Write by other. */
#define TOEXEC      00001       /**< Execute/search by other. */

/**
 * Cypress definitions.
 */

#define CY_UNTAR_CONTEXT_MAGIC      (0x981345A0)            /**< Tag to verify the context.  */
#define CY_MAX_TAR_FILES            (8)                     /**< Max number of files supported in the Tar archive. */
#define CY_TAR_COALESCE_BUFFER_SIZE (TAR_BLOCK_SIZE * 2)    /**< Buffer size used to coalesce sectors if needed. */
#define CY_FILE_TYPE_LEN            (16)                    /**< Max file type string length. */
#define CY_VERSION_STRING_MAX       (16)                    /**< Max version string length "1234.56789.012345". */

typedef enum {
    CY_UNTAR_SUCCESS          = 0,              /**< Untar successful.   */
    CY_UNTAR_ERROR,                             /**< Generic error in the Untar function. */
    CY_UNTAR_INVALID,                           /**< Tar archive is invalid.     */
    CY_UNTAR_NOT_ENOUGH_DATA,                   /**< Not enough data in the Tar archive for all files. */
    CY_UNTAR_COMPONENTS_JSON_PARSE_FAIL         /**< Parsing the components.json file failed. */
} cy_untar_result_t;

typedef enum {
    CY_TAR_PARSE_UNINITIALIZED = 0,             /**< untar uninitialized.    */
    CY_TAR_PARSE_FIND_HEADER,                   /**< Found tar file header. */
    CY_TAR_PARSE_DATA,                          /**< This is not a tar archive; process as data. */
} cy_tar_parse_state_t;


/**
 * @brief Structure defined by IEEE for start of each file within the archive.
 */
typedef struct ustar_header_s {
                                    /*  offset       size                                */
    uint8_t name[TNAMELEN];         /**<    0  0x00   100   File name (NUL terminated)    */
    uint8_t mode[8];                /**<  100  0x64     8   See mode bits above.           */
    uint8_t uid[8];                 /**<  108  0x6c     8   User ID.                      */
    uint8_t gid[8];                 /**<  116  0x74     8   Group ID.                      */
    uint8_t size[12];               /**<  124  0x7c    12   File size.                     */
                                                     /* 12-digit octal in ASCII.         */
                                                     /* Number of logical records   */
                                                     /* written following the header    */
                                                     /* shall be (size+511)/512,        */
                                                     /* ignoring any fraction in the    */
                                                     /* result of the division.         */
    uint8_t mtime[12];              /**<  136  0x88    12   Modified time.                */
    uint8_t chksum[8];              /**<  148  0x94     8   Checksum.                      */
    uint8_t typeflag;               /**<  156  0x9c     1   Type (see above).              */
    uint8_t linkname[TNAMELEN];     /**<  157  0x9d   100   Linked file name.              */
    uint8_t magic[TMAGLEN];         /**<  257 0x101     6   TMAGIC (NUL terminated).      */
    uint8_t version[TVERSLEN];      /**<  263 0x107     2   TVERSION.                      */
    uint8_t uname[32];              /**<  265 0x109    32   user name.                     */
    uint8_t gname[32];              /**<  297 0x129    32   Group name.                    */
    uint8_t devmajor[8];            /**<  329 0x149     8   Major.                         */
    uint8_t devminor[8];            /**<  337 0x151     8   Minor.                         */
    uint8_t prefix[155];            /**<  345 0x159   155   Prefix.                        */
                                    /**<  500 0x1f4    12   Padding implied to the end of the 512-byte block.   */
} ustar_header_t;


/**
 * @brief Structure used for keeping track of files in the tar archive while extracting.
 */
typedef struct cy_ota_file_info_s {
    char                name[TNAMELEN];         /**< Copied from the components.json file         */
    char                type[CY_FILE_TYPE_LEN]; /**< From components.json.                         */
    int                 found_in_tar;           /**< Encountered the header in the tar file.       */
    uint32_t            header_offset;          /**< Offset of the header in the tar file.                 */
    uint32_t            size;                   /**< From components.json, verified from the header.   */
    uint32_t            processed;              /**< Bytes processed from the tar file.                */
} cy_ota_file_info_t;

/**
 * @brief Callback to handle the tar data.
 *
 * @param ctxt          untar context.
 * @param file_index    Index into ctxt->files for the data.
 * @param buffer        Data to use.
 * @param file_offset   Offset into the file to store the data.
 * @param chunk_size    Amount of data in the buffer to use.
 * @param cb_arg        Argument passed into initialization.
 *
 * return   CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
typedef struct cy_untar_context_s * cy_untar_context_ptr;
typedef cy_untar_result_t (*untar_write_callback_t)(cy_untar_context_ptr ctxt, uint16_t file_index, uint8_t *buffer, uint32_t file_offset, uint32_t chunk_size, void *cb_arg);

typedef struct cy_untar_context_s {
    uint32_t                magic;                          /**< CY_UNTAR_CONTEXT_MAGIC.               */
    cy_tar_parse_state_t    state;                          /**< Current parsing state.                 */
    untar_write_callback_t  cb_func;                        /**< Callback function to deal with the data.  */
    void                    *cb_arg;                        /**< Opaque argument passed to callback.        */

    int                     already_parsed_components_json; /**< True if components.json is parsed. */
    uint32_t                bytes_processed;                /**< Bytes processed from the archive.     */

    /* for JSON parsing */
    char                    version[CY_VERSION_STRING_MAX]; /**< String of major.minor.build.          */
    uint16_t                num_files_in_json;              /**< Number of files in components.json.   */
    uint16_t                curr_file_in_json;              /**< Parsing file info in components.json. */

    uint16_t                current_file;                   /**< Currently processing this file from the tar archive.  */
    uint16_t                num_files;                      /**< number of files encountered.                          */
    cy_ota_file_info_t      files[CY_MAX_TAR_FILES];        /**< File info.                                            */

    uint32_t                coalesce_stream_offset;         /**< Offset into the stream where the coalesce buffer starts.  */
    uint32_t                coalesce_bytes;                 /**< Number of bytes in the coalesce buffer.               */
    uint32_t                coalesce_needs;                 /**< Number of bytes needed in the coalesce buffer.        */
    uint8_t                 coalesce_buffer[CY_TAR_COALESCE_BUFFER_SIZE];   /**< Used to coalesce buffers for longer sequential data. */
} cy_untar_context_t;

/**
 * @brief Determine whether this is a tar header.
 *
 * @param buffer            Pointer to the data buffer.
 * @param size              Size of the buffer.
 *
 * @return  CY_UNTAR_NOT_VALID
 *          CY_UNTAR_VALID
 *          CY_UNTAR_NOT_ENOUGH_DATA
 */
cy_untar_result_t cy_is_tar_header( uint8_t *buffer, uint32_t size );

/**
 * @brief Initialize the tar context.
 *
 * @param ctxt              Pointer to the context structure.
 * @param cb_func           Callback function.
 * @param cb_arg            opaque argument passed in callback.
 *
 * @return  CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
cy_untar_result_t cy_untar_init( cy_untar_context_t *ctxt, untar_write_callback_t cb_func, void *cb_arg );

/**
 * @brief De-Initialize the tar context.
 *
 * @param ctxt              Pointer to the context structure.
 *
 * @return  CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
cy_untar_result_t cy_untar_deinit( cy_untar_context_t *ctxt );

/**
 * @brief Parse the tar data.
 *
 * NOTE: This is meant to be called for each chunk of data received.
 *       Callback will be invoked when there is data to write.
 *
 * @param ctxt              Pointer to the context structure; gets updated.
 * @param stream_offset     Byte offset in the current stream of the start of tar_buffer.
 * @param tar_buffer        Pointer to the next buffer of the input.
 * @param size              Bytes in tar_buffer.
 * @param consumed          Bytes used in callbacks (or skipped at end of file).
 *
 * @return  CY_UNTAR_SUCCESS
 *          CY_UNTAR_ERROR
 */
cy_untar_result_t cy_untar_parse( cy_untar_context_t *ctxt, uint32_t stream_offset, uint8_t *tar_buffer, uint32_t size, uint32_t *consumed);


#ifdef __cplusplus
} /*extern "C" */
#endif


#endif  /* UNTAR_H__ */
