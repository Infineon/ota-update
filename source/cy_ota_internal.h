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

#ifndef CY_OTA_INTERNAL_H__
#define CY_OTA_INTERNAL_H__ 1


#ifdef __cplusplus
extern "C" {
#endif

#include "cyabs_rtos.h"
#include "cy_ota_log.h"
#include "untar.h"

/* This is so that Eclipse doesn't complain about the Logging messages */
#ifndef NULL
#define NULL    (void*)0x00
#endif

/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/
#define SECS_TO_MILLISECS(secs)                 (secs * 1000)

/**
 * @brief Tag value used to validate the OTA context
 */
#define CY_OTA_TAG                              (0x0ad38f41)

/**
 * @brief Size of the receive buffer for HTTP
 */
#define CY_OTA_SIZE_OF_RECV_BUFFER              (4 * 1024)

/**
 * @brief Maximum size of signature scheme descriptive string
 *
 *  ex: "sig-sha256-ecdsa"
 */
#define CY_OTA_MAX_SIGN_LENGTH                  (32)

/* Although the events are stored in a 32-bit integer, there are only 24 flags available */
typedef enum
{
    CY_OTA_EVENT_RUNNING_EXITING         = (1 <<  0),    /**< Notify when OTA thread is running / exiting    */
    CY_OTA_EVENT_SHUTDOWN_NOW            = (1 <<  1),    /**< Notify Agent to shutdown                       */

    CY_OTA_EVENT_START_UPDATE            = (1 <<  2),    /**< Start update process                           */

    CY_OTA_EVENT_GOT_DATA                = (1 <<  3),    /**< Notify cy_ota_mqtt_get() or cy_ota_http_get()  */
    CY_OTA_EVENT_DATA_DONE               = (1 <<  4),    /**< Notify cy_ota_get() that we got all the data   */

    CY_OTA_EVENT_DISCONNECT              = (1 <<  5),    /**< Disconnect from Broker / Server                */

    CY_OTA_EVENT_PACKET_TIMEOUT          = (1 <<  6),    /**< Packet download timed out                      */
    CY_OTA_EVENT_DATA_DOWNLOAD_TIMEOUT   = (1 <<  7),    /**< Timed out waiting for download job             */
    CY_OTA_EVENT_STORAGE_ERROR           = (1 <<  8),    /**< Notify cy_ota_get() to exit                    */
    CY_OTA_EVENT_INVALID_VERSION         = (1 <<  9),    /**< Notify cy_ota_get() to exit                    */
    CY_OTA_EVENT_MALFORMED_JOB_DOC       = (1 << 10),    /**< Job JSON document is malformed                 */

    CY_OTA_EVENT_DATA_FAIL               = (1 << 11),    /**< Data failure (could be sign / decrypt fail)    */
    CY_OTA_EVENT_DROPPED_US              = (1 << 12),    /**< MQTT / HTTP Server dropped us                  */

    CY_OTA_EVENT_APP_STOPPED_OTA         = (1 << 13),    /**< MQTT / HTTP "Storage Write" returned OTA_STOP  */


} ota_events_t;

/* OTA Agent main loop events to wait look for */
#define CY_OTA_EVENT_THREAD_EVENTS   (CY_OTA_EVENT_SHUTDOWN_NOW | CY_OTA_EVENT_START_UPDATE )

/* Main OTA Agent loop event wait time */
#define CY_OTA_WAIT_FOR_EVENTS_MS       (CY_RTOS_NEVER_TIMEOUT)


/***********************************************************************
 *
 * Macros
 *
 **********************************************************************/

/**
 * @brief Verify that the OTA Context is valid
 */
#define CY_OTA_CONTEXT_ASSERT(ctx)  CY_ASSERT( (ctx!=NULL) && (ctx->tag==CY_OTA_TAG) )

/**
 * @brief max number of packets to check for missing & duplicate packets
 */
#define CY_OTA_MAX_PACKETS     (2048)  /* Handle checking for 2M OTA code using 1k packets */

/******************************************************************************
 *
 * Include the transport header files
 *
 ******************************************************************************/


/***********************************************************************
 *
 * HTTP
 *
 **********************************************************************/
#ifdef COMPONENT_OTA_HTTP

/*
 * @brief Size of an HTTP header
 *
 * This size needs to be larger than a large HTTP Header. We use this to read
 * part of a Job document. If we try to read CY_OTA_SIZE_OF_RECV_BUFFER
 * bytes, we get a timeout, which causes problems. So we read a smaller amount
 * and read more to get the full Job document.
 */
#define CY_OTA_HTTP_TYPICAL_HEADER_SIZE         (256)

/* OTA MQTT main loop events to wait look for */
#define CY_OTA_EVENT_HTTP_EVENTS  (CY_OTA_EVENT_SHUTDOWN_NOW | \
                                CY_OTA_EVENT_PACKET_TIMEOUT | \
                                CY_OTA_EVENT_DATA_DOWNLOAD_TIMEOUT | \
                                CY_OTA_EVENT_STORAGE_ERROR | \
                                CY_OTA_EVENT_DISCONNECT | \
                                CY_OTA_EVENT_INVALID_VERSION | \
                                CY_OTA_EVENT_GOT_DATA | \
                                CY_OTA_EVENT_DATA_DONE | \
                                CY_OTA_EVENT_DATA_FAIL | \
                                CY_OTA_EVENT_APP_STOPPED_OTA | \
                                CY_OTA_EVENT_DROPPED_US )

/* Inner HTTP event loop - time to wait for events */
#define CY_OTA_WAIT_HTTP_EVENTS_MS      (1)

/* Time to wait for a free Queue entry to pass buffer to MQTT event loop */
#define CY_OTA_WAIT_HTTP_MUTEX_MS       (SECS_TO_MILLISECS(20) )

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/**
 * @brief HTTP context data
 */
typedef struct cy_ota_http_context_s {
    bool                connection_from_app;                        /**< true if HTTP connection passed in from App */
    bool                connection_established;                     /**< true if HTTP connection established        */

    cy_http_client_t    connection;                             /**< HTTP connection instance                   */

    cy_timer_t          http_timer;                             /**< For detecting early end of download        */
    ota_events_t        http_timer_event;                       /**< Event to trigger when timer goes off       */


    char                json_doc[CY_OTA_JSON_DOC_BUFF_SIZE];    /**< Message to request OTA data            */
    char                file[CY_OTA_HTTP_FILENAME_SIZE];        /**< Filename for OTA data                  */

} cy_ota_http_context_t;
#endif /* COMPONENT_OTA_HTTP    */


/***********************************************************************
 *
 * MQTT
 *
 **********************************************************************/

#ifdef COMPONENT_OTA_MQTT
/* OTA MQTT main loop events to wait look for */
#define CY_OTA_EVENT_MQTT_EVENTS  (CY_OTA_EVENT_SHUTDOWN_NOW | \
                                CY_OTA_EVENT_PACKET_TIMEOUT | \
                                CY_OTA_EVENT_DATA_DOWNLOAD_TIMEOUT | \
                                CY_OTA_EVENT_STORAGE_ERROR | \
                                CY_OTA_EVENT_DISCONNECT | \
                                CY_OTA_EVENT_INVALID_VERSION | \
                                CY_OTA_EVENT_GOT_DATA |  \
                                CY_OTA_EVENT_DATA_DONE | \
                                CY_OTA_EVENT_DATA_FAIL | \
                                CY_OTA_EVENT_APP_STOPPED_OTA | \
                                CY_OTA_EVENT_DROPPED_US )

/* Inner MQTT event loop - time to wait for events */
#define CY_OTA_WAIT_MQTT_EVENTS_MS      (CY_RTOS_NEVER_TIMEOUT)

/* Time to wait for MQTT subscription callback Mutex */
#define CY_OTA_WAIT_MQTT_MUTEX_MS       (SECS_TO_MILLISECS(20) )

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/**
 * @brief MQTT context data
 */
typedef struct cy_ota_mqtt_context_s {

    bool                connection_from_app;            /**< true if MQTT connection passed in from App */
    bool                connection_established;          /**< true if MQTT connection established        */

    cy_mqtt_t           mqtt_connection;                 /**< MQTT connection information                */


    cy_timer_t          mqtt_timer;                     /**< For detecting early end of download        */
    ota_events_t        mqtt_timer_event;               /**< Event to trigger when timer goes off       */
    bool                mqtt_timer_inited;              /**< true if MQTT timer initialized             */

    uint8_t             received_packets[CY_OTA_MAX_PACKETS];   /**< keep track of packets for missing / duplicates */

    char                json_doc[CY_OTA_JSON_DOC_BUFF_SIZE];    /**< Message to request OTA data */

    uint8_t             use_unique_topic;               /**< if == 1, create and use unique topic!      */
    char                unique_topic[CY_OTA_MQTT_UNIQUE_TOPIC_BUFF_SIZE]; /**< Topic for receiving OTA data */
    bool                unique_topic_subscribed;        /**< true if UNIQUE MQTT subscription accepted    */
} cy_ota_mqtt_context_t;

#endif /* COMPONENT_OTA_MQTT    */

/***********************************************************************
 *
 * BLUETOOTH
 *
 **********************************************************************/

#ifdef COMPONENT_OTA_BLUETOOTH

#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
#include "ota_ecc_pp.h"
#include "sha256.h"
#endif


/**
 * @brief standard signature length for Bluetooth® key
 */
#define SIGNATURE_LEN               (KEY_LENGTH_BYTES * 2)

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/**
 * @brief Bluetooth® context data
 */
typedef struct {
    uint32_t                    received_crc32;             /**< Bluetooth® CRC sent from the Host                             */
    uint32_t                    crc32;                      /**< Bluetooth® CRC calculated during download                     */
    uint32_t                    file_bytes_written;         /**< Bluetooth® File bytes written                                 */
    uint8_t                     percent;                    /**< Bluetooth® percentage downloaded                              */
    uint8_t                     bt_notify_buff;             /**< Bluetooth® for notifications and indications                  */
#ifdef  COMPONENT_OTA_BLUETOOTH_SECURE
    mbedtls_sha256_context      bt_sha2_ctx;                /**< Bluetooth® For calculating the signature as we download chunks */
    uint8_t                     signature[SIGNATURE_LEN];   /**< Bluetooth® Downloaded signature for the file                   */
    uint32_t                    sig_offset;                 /**< Bluetooth® Signature offset for downloading in pieces          */
#endif
} cy_ota_ble_context_t;

#endif  /* COMPONENT_OTA_BLUETOOTH */

/******************************************************************************
 *
 * Defines
 *
*******************************************************************************/


/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/
/**
 * @brief Structures used for parsing Job JSON info
 *
 * This struct holds the separated fields.
 */
typedef struct cy_ota_job_parsed_info_s {
        char                    job_doc[CY_OTA_JSON_DOC_BUFF_SIZE];         /**< Full Job doc sent from publisher   */
        cy_rslt_t               parse_result;                               /**< Parse result                       */
        /* separated pieces */
        char                    message[CY_OTA_MESSAGE_LEN];                /**< Message ex: "Update Available"     */
        char                    manuf[CY_OTA_JOB_MANUF_LEN];                /**< Manufacturer                       */
        char                    manuf_id[CY_OTA_JOB_MANUF_ID_LEN];          /**< Manufacturer ID                    */
        char                    product[CY_OTA_JOB_PRODUCT_ID_LEN];            /**< Product Name                       */
        char                    serial[CY_OTA_JOB_SERIAL_NUMBER_LEN];       /**< Serial Number                      */
        char                    app_ver[CY_OTA_JOB_VERSION_LEN];            /**< Version "<major>.<minor>.<build>"  */
        uint16_t                ver_major;                                  /**< Major Version of the OTA Image     */
        uint16_t                ver_minor;                                  /**< Minor Version of the OTA Image     */
        uint16_t                ver_build;                                  /**< Build Version of the OTA Image     */
        char                    board[CY_OTA_JOB_BOARD_LEN];                /**< Board (ex: "CYC8PROTO_062_4343W")  */
        cy_ota_connection_t     connect_type;                               /**< Connection type                    */
        char                    new_host_name[CY_OTA_JOB_URL_BROKER_LEN];   /**< New Host Name                      */
#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
        cy_awsport_server_info_t    broker_server;                          /**< Broker or Server holding OTA Image */
#endif
        char                    file[CY_OTA_HTTP_FILENAME_SIZE];            /**< File on Server (HTTP)              */
        uint32_t                file_size;                                  /**< size of file to download           */
        char                    topic[CY_OTA_MQTT_UNIQUE_TOPIC_BUFF_SIZE];  /**< Unique Topic                       */
} cy_ota_job_parsed_info_t;

/**
 * @brief internal OTA Context structure
 */
typedef struct cy_ota_context_s {

    uint32_t                    tag;                        /**< Must be CY_OTA_TAG to be valid                             */

    cy_ota_network_params_t     network_params;             /**< copy of initial connection parameters                      */
    cy_ota_agent_params_t       agent_params;               /**< copy of initial agent parameters                           */
    cy_ota_agent_mem_interface_t flash_iface;               /**< Flash memory handle                                        */
    cy_event_t                  ota_event;                  /**< Event signaling @ref ota_events_t                          */
    cy_thread_t                 ota_agent_thread;           /**< OTA Agent Thread                                           */

    cy_ota_agent_state_t        curr_state;                 /**< current OTA system state                                   */

    uint8_t                     stop_OTA_session;           /**< App callback returned CY_RSLT_OTA_ERROR_APP_RETURNED_STOP  */
    uint32_t                    initial_timer_sec;          /**< Seconds before connecting after cy_ota_agent_start()       */
    uint32_t                    next_timer_sec;             /**< Seconds between connect after successful download          */
    uint32_t                    retry_timer_sec;            /**< Seconds between connect retry after failed download        */
    uint32_t                    job_check_timeout_sec;      /**< Seconds to wait when deciding Job server failed download   */
    uint32_t                    data_check_timeout_sec;     /**< Seconds to wait when deciding Data server failed download  */
    uint32_t                    check_timeout_sec;          /**< Seconds to wait for server to start download               */
    uint32_t                    packet_timeout_sec;         /**< Seconds to wait between packets for download               */
    uint16_t                    ota_retries;                /**< count # retries between initial or next intervals          */

    cy_timer_t                  ota_timer;                  /**< for delaying start of connections      */
    ota_events_t                ota_timer_event;            /**< event to trigger when timer goes off   */

    /* Storage and progress info */
    void                        *storage_loc;               /**< can be cast as flash_area or FILE as needed                    */
    uint32_t                    total_image_size;           /**< Total size of OTA Image                                        */
    uint32_t                    total_bytes_written;        /**< Number of bytes written to FLASH                               */
    uint32_t                    last_offset;                /**< Last offset written to from cy_ota_storage_write()             */
    uint32_t                    last_size;                  /**< last size of data written from cy_ota_storage_write()          */
    uint16_t                    last_packet_received;       /**< Last Packet of data we have received                           */
    uint16_t                    total_packets;              /**< Total number of Packets of data for the OTA Image              */
    uint16_t                    num_packets_received;       /**< Total number of Packets received                               */
    uint16_t                    last_num_packets_received;  /**< last time we saw how many were received, per-packet timer      */

    cy_mutex_t                  sub_callback_mutex;         /**< Keep subscription callbacks from being time-sliced             */
    uint8_t                     sub_callback_mutex_inited;  /**< 1 = sub_callback_mutex initialized                             */

    /* Network connection */
    int                         contact_server_retry_count; /**< Keep count of # tries to contact server                        */
    int                         download_retry_count;       /**< Keep count of # tries to download update                       */

    uint8_t                     reboot_after_sending_result;    /**< Only reboot after verified and result sent                 */
    uint8_t                     app_connected;              /**< Keep track of the App making the connection                    */
    uint8_t                     device_connected;           /**< Keep track of if the Device is connected (OTA Agent or APP)    */
    cy_ota_connection_t         curr_connect_type;          /**< Connection Type (may change after parsing Job)                 */
#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
    cy_awsport_server_info_t    *curr_server;               /**< current server information (may change after parsing Job)      */
#endif
#ifdef COMPONENT_OTA_MQTT
    cy_ota_mqtt_context_t       mqtt;                       /**< MQTT specific context data                                     */
#endif
#ifdef COMPONENT_OTA_HTTP
    cy_ota_http_context_t       http;                       /**< HTTP specific context data                                     */
#endif
#ifdef COMPONENT_OTA_BLUETOOTH
    cy_ota_ble_context_t        ble;                        /**< Bluetooth® specific context data                               */
#endif
#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
    uint8_t                     data_buffer[CY_OTA_SIZE_OF_RECV_BUFFER];    /**< Used to get Job and Data                       */
    char                        job_doc[CY_OTA_JSON_DOC_BUFF_SIZE];         /**< Message to parse                               */
    cy_ota_job_parsed_info_t    parsed_job;                                 /**< Parsed Job JSON info                           */

    uint8_t                     chunk_buffer[CY_OTA_CHUNK_SIZE + 512];    /**< Store Chunked data here */
#endif
    cy_ota_cb_struct_t          callback_data;              /**< For passing data to callback function                          */

    cy_ota_storage_write_info_t *storage;                   /**< pointer to a chunk of data to write                            */
    uint8_t                     storage_open;               /**< 1 = storage is open                                            */
    uint8_t                     ota_is_tar_archive;         /**< !=0, this is a tar file                                        */
} cy_ota_context_t;

/***********************************************************************
 *
 * Variables
 *
 **********************************************************************/
extern CY_LOG_LEVEL_T ota_logging_level;

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/
/**
 * @brief OTA set internal state
 *
 * @param   ctx     - OTA context
 * @param   state   - new OTA state
 *
 * @return  N/A
 */
void cy_ota_set_state(cy_ota_context_t *ctx, cy_ota_agent_state_t state);

/* --------------------------------------------------------------- */

/**
 * @brief OTA internal Callback to User
 *
 * @param   ctx     - OTA context
 * @param   reason  - OTA Callback reason
 *
 * @return  N/A
 */
cy_ota_callback_results_t cy_ota_internal_call_cb(cy_ota_context_t *ctx,
                                                  cy_ota_cb_reason_t reason,
                                                  cy_ota_agent_state_t report_state);

/***********************************************************************
 *
 * OTA Network abstraction
 *
 **********************************************************************/
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
cy_rslt_t cy_ota_http_validate_network_params(const cy_ota_network_params_t *network_params);
cy_rslt_t cy_ota_mqtt_validate_network_params(const cy_ota_network_params_t *network_params);
cy_rslt_t cy_ota_ble_validate_network_params(const cy_ota_network_params_t *network_params);



/**
 * @brief Connect to OTA Update server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_connect(cy_ota_context_t *ctx);
cy_rslt_t cy_ota_mqtt_connect(cy_ota_context_t *ctx);

/**
 * @brief get the OTA download
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_get_job(cy_ota_context_t *ctx);
cy_rslt_t cy_ota_mqtt_get_job(cy_ota_context_t *ctx);

/**
 * @brief get the OTA download
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_get_data(cy_ota_context_t *ctx);
cy_rslt_t cy_ota_mqtt_get_data(cy_ota_context_t *ctx);

/**
 * @brief Disconnect from Broker/Server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_disconnect(cy_ota_context_t *ctx);
cy_rslt_t cy_ota_mqtt_disconnect(cy_ota_context_t *ctx);

/**
 * @brief Report OTA result to MQTT Broker or HTTP Server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_report_result(cy_ota_context_t *ctx, cy_rslt_t last_error);
cy_rslt_t cy_ota_mqtt_report_result(cy_ota_context_t *ctx, cy_rslt_t last_error);



/**
 * @brief Create a unique topic name
 *
 * Do not create a new one if one is already created
 */
void cy_ota_mqtt_create_unique_topic(cy_ota_context_t *ctx);


/**********************************************************************
 *
 * Utility functions
 *
 **********************************************************************/

void cy_ota_print_data(const char *buffer, uint32_t length);

#ifdef __cplusplus
    }
#endif

#endif /* CY_OTA_INTERNAL_H__ */
