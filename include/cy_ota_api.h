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

/** @file
 *
 * The Cypress OTA API abstracts the underlying network and
 * platform support for Over-The-Air updates.
 */

/**
 * \addtogroup group_cy_ota Infineon Over The Air (OTA) API
 * \{
 * OTA support for downloading and installing firmware updates.
 *
 * \defgroup group_ota_macros OTA Macros
 * Macros used to define the OTA Agent behavior.
 *
 * \defgroup group_ota_typedefs OTA Typedefs
 * Typedefs used by the OTA Agent.
 *
 * \defgroup group_ota_callback OTA Agent Callback
 * An application can get callbacks from the OTA Agent.
 *
 * \defgroup group_ota_structures OTA Structures
 * Structures used for passing information to and from OTA Agent.
 *
 * \defgroup group_ota_functions OTA Functions
 * Functions to start/stop and query the OTA Agent.
 *
 */

/**
 *
 *
 **********************************************************************/

#ifndef CY_OTA_API_H__
#define CY_OTA_API_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "cy_ota_config.h"          /* Customer OTA overrides.   */
#include "cy_ota_defaults.h"        /* Defaults for OTA.         */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_result_mw.h"
#include "cy_log.h"

/*
 * To use WiFi-HTTP support, add this to the makefile.
 * COMPONENT+=OTA_HTTP
 */

/*
 * To use WiFi-MQTT support, add this to the makefile.
 * COMPONENT+=OTA_MQTT
 */

/*
 * To use Bluetooth®, add this define to the makefile.
 * Note that application must provide Bluetooth® initialization and callback handling.
 * Please see full documentation for code snippets.
 * COMPONENT+=OTA_BLUETOOTH
 */

#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
#include "cybsp_wifi.h"
#include "cy_tcpip_port_secure_sockets.h"
#endif

#ifdef COMPONENT_OTA_HTTP
#include "cy_http_client_api.h"
#endif

#ifdef COMPONENT_OTA_MQTT
#include "cy_mqtt_api.h"
#endif

/* We need another include for Bluetooth® support */
#ifdef COMPONENT_OTA_BLUETOOTH
#include "wiced_bt_gatt.h"
#endif

/**
 * @brief - Reuse CYLF_AUDIO for OTA logging.
 */
#define CYLF_OTA    CYLF_AUDIO

 /**
  * \addtogroup group_ota_macros
  * \{
  */

/***********************************************************************
 * Error Defines
 ***********************************************************************/

/* Cypress middleware APIs return results of type cy_rslt_t and consist of three parts:
* - module base
* - type
* - error code
*
* \par Result Format
*
  \verbatim
             Module base         Type    Library-specific error code
     +-------------------------+------+------------------------------+
     |          0x0200         | 0x2  |           Error Code         |
     +-------------------------+------+------------------------------+
               14 bits          2 bits            16 bits

  See the macro section of this document for library-specific error codes.
  \endverbatim
*
* The data structure cy_rslt_t is part of cy_result.h.
* In MTB, the PSoC 6 MCU target platform is located in <core_lib/include>.
*
* Module base: This base is derived from CY_RSLT_MODULE_MIDDLEWARE_BASE (defined in cy_result.h) and is an offset of CY_RSLT_MODULE_MIDDLEWARE_BASE.
*              Details of the offset and the middleware base are defined in cy_result_mw.h, which is part of [Github connectivity-utilities] (https://github.com/cypresssemiconductorco/connectivity-utilities).
*              For example, OTA uses CY_RSLT_MODULE_MIDDLEWARE_OTA_UPDATE as the module base, which is 0x020D.
*
* Type: This type is defined in cy_result.h and can be one of CY_RSLT_TYPE_FATAL, CY_RSLT_TYPE_ERROR, CY_RSLT_TYPE_WARNING, or CY_RSLT_TYPE_INFO. MQTT library error codes are of type CY_RSLT_TYPE_ERROR which is 0x2.
*
* Library-specific error codes: These error codes are library-specific and defined in the macro section.
*
* Helper macros used for creating library-specific results are provided as part of cy_result.h.
*/

#define CY_RSLT_OTA_ERROR_BASE    CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_MIDDLEWARE_OTA_UPDATE, 0) /**< OTA Update error code base. */

#define CY_RSLT_OTA_ERROR_UNSUPPORTED           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  1) ) /**< Unsupported feature.                  */
#define CY_RSLT_OTA_ERROR_GENERAL               ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  2) ) /**< Generic error.                        */
#define CY_RSLT_OTA_ERROR_BADARG                ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  3) ) /**< Bad argument.                         */
#define CY_RSLT_OTA_ERROR_OUT_OF_MEMORY         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  4) ) /**< Out of Memory error.                  */
#define CY_RSLT_OTA_ERROR_ALREADY_STARTED       ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  5) ) /**< OTA update already started.           */
#define CY_RSLT_OTA_ERROR_MQTT_INIT             ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  6) ) /**< MQTT init failed.                     */
#define CY_RSLT_OTA_ERROR_OPEN_STORAGE          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  7) ) /**< Could not open local storage.         */
#define CY_RSLT_OTA_ERROR_READ_STORAGE          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  8) ) /**< Could not Read from local storage.    */
#define CY_RSLT_OTA_ERROR_WRITE_STORAGE         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  9) ) /**< Could not Write to local storage.     */
#define CY_RSLT_OTA_ERROR_CLOSE_STORAGE         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 10) ) /**< Close local storage error.            */
#define CY_RSLT_OTA_ERROR_CONNECT               ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 11) ) /**< Connection to the server failed.      */
#define CY_RSLT_OTA_ERROR_DISCONNECT            ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 12) ) /**< Server disconnect error.              */
#define CY_RSLT_OTA_ERROR_REDIRECT              ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 13) ) /**< Redirection failure.                  */
#define CY_RSLT_OTA_ERROR_SERVER_DROPPED        ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 14) ) /**< Broker/server disconnected.           */
#define CY_RSLT_OTA_ERROR_MQTT_SUBSCRIBE        ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 15) ) /**< Broker/server Subscribe error.        */
#define CY_RSLT_OTA_ERROR_MQTT_PUBLISH          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 16) ) /**< Broker/Server Publish error.          */
#define CY_RSLT_OTA_ERROR_GET_JOB               ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 17) ) /**< Failed to get the OTA Job document.   */
#define CY_RSLT_OTA_ERROR_GET_DATA              ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 18) ) /**< Failed to get the OTA image.          */
#define CY_RSLT_OTA_ERROR_NOT_A_HEADER          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 19) ) /**< No header in the payload.             */
#define CY_RSLT_OTA_ERROR_NOT_A_JOB_DOC         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 20) ) /**< Job document is invalid.              */
#define CY_RSLT_OTA_ERROR_MALFORMED_JOB_DOC     ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 21) ) /**< Malformed Job document.               */
#define CY_RSLT_OTA_ERROR_WRONG_BOARD           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 22) ) /**< Board in Job document does not match. */
#define CY_RSLT_OTA_ERROR_INVALID_VERSION       ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 23) ) /**< Invalid version in Job or Packet.     */
#define CY_RSLT_OTA_ERROR_VERIFY                ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 24) ) /**< Verify image failure.                 */
#define CY_RSLT_OTA_ERROR_SENDING_RESULT        ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 25) ) /**< Sending result failed.                */
#define CY_RSLT_OTA_ERROR_APP_RETURNED_STOP     ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 26) ) /**< Callback returned Stop OTA download.  */
#define CY_RSLT_OTA_ERROR_APP_EXCEEDED_RETRIES  ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 27) ) /**< Number of retries exceeded.           */
#define CY_RSLT_OTA_ERROR_TRANSPORT_UNSUPPORTED ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 28) ) /**< HTTP, MQTT or Bluetooth® not supported in this build. */

#if defined(COMPONENT_OTA_BLUETOOTH) || defined(CY_DOXYGEN)
#define CY_RSLT_OTA_ERROR_BLE_GENERAL           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 32) ) /**< General Bluetooth® error.                     */
#define CY_RSLT_OTA_ERROR_BLE_VERIFY            ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 33) ) /**< Bluetooth® Verification of download error.    */
#define CY_RSLT_OTA_ERROR_BLE_GATT              ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 34) ) /**< Bluetooth® GATT event error.                  */
#endif

#define CY_RSLT_OTA_ERROR_SMIF_TIMEOUT          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 64) ) /**< SMIF timeout.             */

#define CY_RSLT_OTA_INFO_BASE    CY_RSLT_CREATE(CY_RSLT_TYPE_INFO, CY_RSLT_MODULE_MIDDLEWARE_OTA_UPDATE, 0)   /**< Base for informational results. */
#define CY_RSLT_OTA_EXITING                     ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 1) ) /**< OTA Agent exiting.                         */
#define CY_RSLT_OTA_ALREADY_CONNECTED           ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 2) ) /**< OTA already connected.                     */
#define CY_RSLT_OTA_CHANGING_SERVER             ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 3) ) /**< Data broker/server different from job.     */

#define CY_RSLT_OTA_USE_JOB_FLOW                ( (cy_rslt_t)(CY_RSLT_SUCCESS          ) ) /**< Use Job flow for update.                 */
#define CY_RSLT_OTA_USE_DIRECT_FLOW             ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 4) ) /**< Use Direct flow for update.              */
#define CY_RSLT_OTA_NO_UPDATE_AVAILABLE         ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 5) ) /**< No OTA update on the server.             */

/** \} group_ota_macros */

/***********************************************************************
 * Define min / max values
 ***********************************************************************/

/**
  * \addtogroup group_ota_typedefs
  * \{
  */

/**
 * @brief Macro combo to add quotes around a define.
 * Used to define a string value for CY_TARGET_BOARD.
 * */
#define ADD_QUOTES(str) #str
/**
 * @brief Macro combo to add quotes around a define.
 * Used to define a string value for CY_TARGET_BOARD.
 * */
#define EXPAND_AND_ADD_QUOTES(str) ADD_QUOTES(str)
/**
 * @brief Macro combo to add quotes around a define.
 * Used to define a string value for CY_TARGET_BOARD.
 * */
#define CY_TARGET_BOARD_STRING EXPAND_AND_ADD_QUOTES(CY_TARGET_BOARD)

/**
 * @brief Size for the HTTP filename to get the OTA image.
 */
#define CY_OTA_MQTT_FILENAME_BUFF_SIZE          (256)


/**
 * @brief Size for the unique MQTT topic to get the OTA image.
 */
#define CY_OTA_MQTT_UNIQUE_TOPIC_BUFF_SIZE      (256)

/**
 * @brief Size of buffer for JSON update document
 */
#define CY_OTA_JSON_DOC_BUFF_SIZE               (1024)

/**
 * @brief Size of data chunks for each transfer.
 */
#define CY_OTA_CHUNK_SIZE                       (4096)

/**
 * @brief Size of the string for the HTTP file name.
 */
#define CY_OTA_HTTP_FILENAME_SIZE               (256)

/**
 * @brief First part of the topic to subscribe / publish.
 *
 * Topic for the device to send messages to the Publisher:
 *  "COMPANY_TOPIC_PREPEND / BOARD_NAME / PUBLISHER_LISTEN_TOPIC"
 *
 * Override in cy_ota_config.h.
 */
#ifndef COMPANY_TOPIC_PREPEND
#define COMPANY_TOPIC_PREPEND                   "OTAUpdate"
#endif

/**
 * @brief Last part of the topic to subscribe.
 *
 * Topic for the device to send messages to the Publisher:
 *  "COMPANY_TOPIC_PREPEND / BOARD_NAME / PUBLISHER_LISTEN_TOPIC"
 *
 * Override in cy_ota_config.h.
 */
#ifndef PUBLISHER_LISTEN_TOPIC
#define PUBLISHER_LISTEN_TOPIC                  "publish_notify"
#endif

/**
 * @brief "Magic" value placed in the MQTT Data Payload header.
 *
 * Override in cy_ota_config.h.
 */
#ifndef CY_OTA_MQTT_MAGIC
#define CY_OTA_MQTT_MAGIC                       "OTAImage"
#endif

/**
 * @brief End of topic to send a message to the Publisher for Direct download.
 *
 * Override in cy_ota_config.h.
 */
#ifndef PUBLISHER_DIRECT_TOPIC
#define PUBLISHER_DIRECT_TOPIC                  "OTAImage"
#endif

/**
 * @brief Publisher response when no update is available.
 */
#define NOTIFICATION_RESPONSE_NO_UPDATES        "No Update Available"

/**
 * @brief Publisher response when an update is available.
 */
#define NOTIFICATION_RESPONSE_UPDATES           "Update Available"

/**
 * @brief Publisher response when receiving results.
 */
#define NOTIFICATION_RESPONSE_RESULT_RECEIVED   "Result Received"

/**
 * @brief Topic to receive messages from the Publisher.
 */
#define CY_OTA_SUBSCRIBE_AVAIL_TOPIC        COMPANY_TOPIC_PREPEND "/" CY_TARGET_BOARD_STRING "/" DEVICE_LISTEN_TOPIC

/**
 * @brief Topic to send messages to the Publisher.
 */
#define SUBSCRIBER_PUBLISH_TOPIC            COMPANY_TOPIC_PREPEND "/" CY_TARGET_BOARD_STRING "/" PUBLISHER_LISTEN_TOPIC

/**
 * @brief Minimum interval check time.
 *
 * Minimum interval time for any timing specification in seconds.
 */
#define CY_OTA_INTERVAL_SECS_MIN        (5)

/**
 * @brief Maximum interval check time.
 *
 * This applies to:
 *  Initial wait after call to cy_ota_agent_start() before contacting server.
 *  Wait value after failing or completing an OTA download.
 */
#define CY_OTA_INTERVAL_SECS_MAX        (60 * 60 * 24 * 365)     /* one year */

/*
 * Check for valid define values.
 */
#if (CY_OTA_INITIAL_CHECK_SECS < CY_OTA_INTERVAL_SECS_MIN)
    #error  "CY_OTA_INITIAL_CHECK_SECS must be greater or equal to CY_OTA_INTERVAL_SECS_MIN."
#endif
#if (CY_OTA_INITIAL_CHECK_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_INITIAL_CHECK_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

#if (CY_OTA_NEXT_CHECK_INTERVAL_SECS < CY_OTA_INTERVAL_SECS_MIN)
    #error  "CY_OTA_NEXT_CHECK_INTERVAL_SECS must be greater or equal to CY_OTA_INTERVAL_SECS_MIN."
#endif
#if (CY_OTA_NEXT_CHECK_INTERVAL_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_NEXT_CHECK_INTERVAL_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

/* Don't check the minimum because this number can be 0. */
#if (CY_OTA_CHECK_TIME_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_CHECK_TIME_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

#if (CY_OTA_RETRY_INTERVAL_SECS < CY_OTA_INTERVAL_SECS_MIN)
    #error  "CY_OTA_RETRY_INTERVAL_SECS must be greater or equal to CY_OTA_INTERVAL_SECS_MIN."
#endif
#if (CY_OTA_RETRY_INTERVAL_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_RETRY_INTERVAL_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

#if (CY_OTA_PACKET_INTERVAL_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_PACKET_INTERVAL_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/

/**
 * @brief The Message Name field in a JSON Job document.
 *
 * The Message Name field indicates the type of request/info that is contained in the Job document.
 */
#define CY_OTA_MESSAGE_FIELD                "Message"

/**
 * @brief The Manufacturer Name field in a JSON Job document.
 *
 * The Manufacturer Name can be used to determine if the OTA image available is correct.
 */
#define CY_OTA_MANUF_FIELD                  "Manufacturer"

/**
 * @brief The Manufacturer ID field in a JSON Job document.
 *
 * The Manufacturer ID can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_MANUF_ID_FIELD               "ManufacturerID"

/**
 * @brief The Product ID field in a JSON Job document.
 *
 * The Product ID can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_PRODUCT_FIELD                "Product"

/**
 * @brief The Serial Number field in a JSON Job document.
 *
 * The Serial Number can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_SERIAL_NUMBER_FIELD          "SerialNumber"

/**
 * @brief The Version field in a JSON Job document.
 *
 * The Version can be used to determine whether the OTA image available is an upgrade (for example,"1.6.3").
 */
#define CY_OTA_VERSION_FIELD                "Version"

/**
 * @brief The Board field in a JSON Job document.
 *
 * The Board can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_BOARD_FIELD                  "Board"

/**
 * @brief The Connection field in a JSON Job document.
 *
 * The Connection defines the connection type to use to obtain the OTA image ("HTTP" or "MQTT").
 */
#define CY_OTA_CONNECTION_FIELD             "Connection"

/**
 * @brief The Broker field for MQTT connection in a JSON Job document.
 *
 * MQTT is the MQTT Broker used to obtain the OTA image.
 */
#define CY_OTA_BROKER_FIELD                 "Broker"

/**
 * @brief The Port field in a JSON Job document.
 *
 * Port is used for HTTP or MQTT connection to obtain the OTA image.
 */
#define CY_OTA_PORT_FIELD                   "Port"

/**
 * @brief The Server field is for HTTP connection in a JSON Job document.
 *
 * The server is the HTTP server used to obtain the OTA image. See also @ref CY_OTA_PORT_FIELD @ref CY_OTA_FILE_FIELD.
 */
#define CY_OTA_SERVER_FIELD                 "Server"

/**
 * @brief The File field is for HTTP connection in a JSON Job document.
 *
 * The File is the name of the OTA image on the HTTP server. See also @ref CY_OTA_PORT_FIELD.
 */
#define CY_OTA_FILE_FIELD                   "File"

/**
 * @brief The Offset field is for a JSON Chunk Request document.
 *
 * The offset is where in the OTA image to start a chunk transfer.
 */
#define CY_OTA_OFFSET_FIELD                 "Offset"

/**
 * @brief The Size field is for a JSON Chunk Request document.
 *
 * The Size is the size of the chunk to read from the OTA image for a chunk transfer.
 */
#define CY_OTA_SIZE_FIELD                   "Size"

/**
 * @brief The Unique Topic field is the name of the topic used for receiving Job/data from the Publisher.
 *
 * The Unique Topic Name used for receipt of the Job/data from the Publisher.
 */
#define CY_OTA_UNIQUE_TOPIC_FIELD           "UniqueTopicName"

/**
 * @brief The MQTT Connection Type used in a JSON Job document.
 *
 * Test for this string in a JSON Job document for MQTT type connection. @ref CY_OTA_CONNECTION_FIELD.
 */
#define CY_OTA_MQTT_STRING                  "MQTT"
/**
 * @brief The HTTP connection type used in a JSON Job document.
 *
 * Test for this string in a JSON Job document for HTTP type connection. @ref CY_OTA_CONNECTION_FIELD.
 */
#define CY_OTA_HTTP_STRING                  "HTTP"
/**
 * @brief The HTTPS connection type used in a JSON Job document.
 *
 * Test for this string in a JSON Job document for HTTPS type connection. @ref CY_OTA_CONNECTION_FIELD.
 */
#define CY_OTA_HTTPS_STRING                 "HTTPS"

/**
 *  @brief The Max length of the "Message" field in a JSON Job document.
 */
#define CY_OTA_MESSAGE_LEN                  (32)

/**
 *  @brief The Max length of the "Manufacturer" field in a JSON Job document.
 */
#define CY_OTA_JOB_MANUF_LEN                (64)

/**
 *  @brief The Max length of the "ManufacturerID" field in a JSON Job document.
 */
#define CY_OTA_JOB_MANUF_ID_LEN             (16)

/**
 *  @brief The Max length of the "ProductID" field in a JSON Job document.
 */
#define CY_OTA_JOB_PRODUCT_ID_LEN           (64)

/**
 *  @brief The Max length of the "SerialNumber" field in a JSON Job document.
 */
#define CY_OTA_JOB_SERIAL_NUMBER_LEN        (32)

/**
 *  @brief The Max length of the "Version" field in a JSON Job document.
 */
#define CY_OTA_JOB_VERSION_LEN              (16)

/**
 *  @brief The Max length of the "Board" field in a JSON Job document.
 */
#define CY_OTA_JOB_BOARD_LEN                (32)

/**
 *  @brief The Max length of the "Broker" and "Server" fields in a JSON Job document.
 */
#define CY_OTA_JOB_URL_BROKER_LEN           (256)

/**
 * @brief The MQTT Broker port for a non-TLS connection.
 */
#define CY_OTA_MQTT_BROKER_PORT             (1883)

/**
 * @brief The MQTT Broker port for a TLS connection.
 */
#define CY_OTA_MQTT_BROKER_PORT_TLS         (8883)

/**
 * @brief The MQTT Broker port for a TLS connection with certificates.
 */
#define CY_OTA_MQTT_BROKER_PORT_TLS_CERT    (8884)


/**
 * @brief The HTTP connection port for a non-TLS connection.
 */
#define CY_OTA_HTTP_SERVER_PORT             (80)

/**
 * @brief The HTTP connection port for a TLS connection.
 */
#define CY_OTA_HTTP_SERVER_PORT_TLS         (443)


/**
 * @brief The Type of OTA update flow.
 * Direct Flow: The application knows the location of the update and directly downloads it.
 * Job Flow: The application loads a Job document which describes where the update is located.
 */
typedef enum
{
    CY_OTA_JOB_FLOW = 0,                /**< Use the Job flow to get the OTA Job document and data.  */
    CY_OTA_DIRECT_FLOW                  /**< Get the OTA image data directly.                        */
} cy_ota_update_flow_t;
/**
 * @brief The connection type to use.
 */
typedef enum
{
    CY_OTA_CONNECTION_UNKNOWN = 0,      /**< Unknown connection type.       */
    CY_OTA_CONNECTION_MQTT,             /**< Use MQTT connection.           */
    CY_OTA_CONNECTION_HTTP,             /**< Use HTTP connection.           */
    CY_OTA_CONNECTION_HTTPS,            /**< Use HTTPS connection.          */
    CY_OTA_CONNECTION_BLE,              /**< Use Bluetooth® connection       */
} cy_ota_connection_t;

/**
 * @brief The MQTT session clean flag.
 *
 * This flag signals the MQTT Broker to start a new session or restart an existing session.
 */
typedef enum
{
    CY_OTA_MQTT_SESSION_CLEAN = 0,      /**< Start a clean MQTT session with the Broker.         */
    CY_OTA_MQTT_SESSION_RESTART         /**< Restart an existing MQTT session with the Broker.   */
} cy_ota_mqtt_session_type_t;


/**
 * @brief The reasons for OTA callbacks to the application.
 */
typedef enum
{
    CY_OTA_REASON_STATE_CHANGE = 0,     /**< OTA Agent state changed, see cb_data->state.    */
    CY_OTA_REASON_SUCCESS,              /**< State function successful.                      */
    CY_OTA_REASON_FAILURE,              /**< State function failed.                          */

    CY_OTA_LAST_REASON                  /**< Placeholder, Do not use.                        */
} cy_ota_cb_reason_t;

/**
 * @brief The OTA state.
 */
typedef enum
{
    CY_OTA_STATE_NOT_INITIALIZED= 0,    /**< OTA system is not initialized.                          */
    CY_OTA_STATE_EXITING,               /**< OTA system is exiting.                                  */
    CY_OTA_STATE_INITIALIZING,          /**< OTA system is initializing.                             */
    CY_OTA_STATE_AGENT_STARTED,         /**< OTA Agent has started.                                  */
    CY_OTA_STATE_AGENT_WAITING,         /**< OTA Agent is waiting for the timer to start.            */

    CY_OTA_STATE_STORAGE_OPEN,          /**< OTA Agent will call cy_storage_open.                    */
    CY_OTA_STATE_STORAGE_WRITE,         /**< OTA Agent will call cy_storage_write.                   */
    CY_OTA_STATE_STORAGE_CLOSE,         /**< OTA Agent will call cy_storage_close.                   */

    CY_OTA_STATE_START_UPDATE,          /**< OTA Agent will determine the flow: Job or Direct.       */

    CY_OTA_STATE_JOB_CONNECT,           /**< OTA Agent will connect to the Job Broker/server.        */
    CY_OTA_STATE_JOB_DOWNLOAD,          /**< OTA Agent will get the Job from the Broker/server.      */
    CY_OTA_STATE_JOB_DISCONNECT,        /**< OTA Agent will disconnect from the Job Broker/server.   */

    CY_OTA_STATE_JOB_PARSE,             /**< OTA Agent will parse the Job.                           */
    CY_OTA_STATE_JOB_REDIRECT,          /**< OTA Agent will use the Job to change the Broker/server. */

    CY_OTA_STATE_DATA_CONNECT,          /**< OTA Agent will connect to the data Broker/server.       */
    CY_OTA_STATE_DATA_DOWNLOAD,         /**< OTA Agent will download the data.                       */
    CY_OTA_STATE_DATA_DISCONNECT,       /**< OTA Agent will disconnect from the data Broker/server.  */

    CY_OTA_STATE_VERIFY,                /**< OTA Agent will verify the download.                     */

    CY_OTA_STATE_RESULT_REDIRECT,       /**< OTA Agent will redirect back to the initial connection. */

    CY_OTA_STATE_RESULT_CONNECT,        /**< OTA Agent will connecting to the result Broker/server.  */
    CY_OTA_STATE_RESULT_SEND,           /**< OTA Agent will send the result.                         */
    CY_OTA_STATE_RESULT_RESPONSE,       /**< OTA Agent will wait for a result response.              */
    CY_OTA_STATE_RESULT_DISCONNECT,     /**< OTA Agent will disconnect from the result Broker/server.*/

    CY_OTA_STATE_OTA_COMPLETE,          /**< OTA Agent is done with current session.
                                             OTA Agent will reboot or will wait.                     */

    CY_OTA_NUM_STATES                   /**< Not used, placeholder. */
} cy_ota_agent_state_t;

#ifdef COMPONENT_OTA_BLUETOOTH

/* Command definitions for the OTA FW upgrade */
#define CY_OTA_UPGRADE_COMMAND_PREPARE_DOWNLOAD               1
#define CY_OTA_UPGRADE_COMMAND_DOWNLOAD                       2
#define CY_OTA_UPGRADE_COMMAND_VERIFY                         3
#define CY_OTA_UPGRADE_COMMAND_ABORT                          4

/* Event definitions for the OTA FW upgrade */
#define CY_OTA_UPGRADE_STATUS_OK                              0
#define CY_OTA_UPGRADE_STATUS_BAD                             1

/* Starting value for crc file check */
#define CRC32_INITIAL_VALUE             0x00000000

#endif  /* COMPONENT_OTA_BLUETOOTH */

/**
 *  The OTA context pointer.
 *
 * Returned from cy_ota_agent_start() and used for subsequent calls.
 */
typedef void *cy_ota_context_ptr;

/**
 * @brief Results from the registered OTA callback.
 *
 * These are the return values from the application to the OTA Agent at the end of a callback.
 *
 * Of the callback reasons, \ref cy_ota_cb_reason_t, the OTA Agent checks only the application
 * callback return values when the reason is CY_OTA_REASON_STATE_CHANGE. The other callback reasons
 * are informational for the application.
 *
 * When the callback reason is CY_OTA_REASON_STATE_CHANGE, the OTA Agent is about to call the function
 * to act on the new state. The application callback function can return a value that affects the
 * OTA Agent.
 *
 * There are a few state changes that will pick up changes made by the application and use those changes.
 *
 *      cb_data->reason             - Reason for the callback.
 *      cb_data->cb_arg             - Argument provided to cy_ota_agent_start() params.
 *      cb_data->ota_agt_state      - CY_OTA_REASON_STATE_CHANGE:
 *                                      - Start of cy_ota_agent_state_t, about to call the default function.
 *                                   - CY_OTA_REASON_SUCCESS:
 *                                      - Error function succeeded.
 *                                   - CY_OTA_REASON_FAILURE:
 *                                      - Error function failed.
 *      cb_data->error              - Same as cy_ota_get_last_error().
 *      cb_data->storage            - For the CY_OTA_STATE_STORAGE_WRITE callback, points to the storage info.
 *      cb_data->total_size         - Total # bytes to be downloaded.
 *      cb_data->bytes_written      - Total # bytes written.
 *      cb_data->percentage         - Percentage of data downloaded.
 *      cb_data->connection_type    - Current connection - MQTT or HTTP.
 *      cb_data->broker_server      - Current server URL and port.
 *      cb_data->credentials        - Pointer to the current credentials being used.
 *      cb_data->mqtt_connection    - Current MQTT connection (NULL if not connected):
 *                                      - For CY_OTA_REASON_STATE_CHANGE
 *                                        - CY_OTA_STATE_JOB_CONNECT or
 *                                          CY_OTA_STATE_DATA_CONNECT or
 *                                          CY_OTA_STATE_RESULT_CONNECT
 *                                      - OTA Agent about to connect, if Application fills this in;
 *                                        then the OTA Agent will use the provided connection instance.
 *      cb_data->http_connection    - Current HTTP connection (NULL if not conencted).
 *                                      - For CY_OTA_REASON_STATE_CHANGE:
 *                                        - CY_OTA_STATE_JOB_CONNECT or
 *                                          CY_OTA_STATE_DATA_CONNECT or
 *                                          CY_OTA_STATE_RESULT_CONNECT
 *                                      - OTA Agent about to connect, if Application fills this in;
 *                                        then the OTA Agent will use the provided connection instance.
 *      cb_data->file               - Filename for HTTP GET command.
 *      cb_data->unique_topic       - Unique topic name to receive data from MQTT Broker/Publisher.
 *      cb_data->json_doc           - JSON document used for:
 *                                      MQTT:
 *                                      - Send to request a Job from the MQTT Broker.
 *                                      - Received Job from the MQTT Broker.
 *                                      - Send to request data from the MQTT Broker.
 *                                      - Send to report the result to the MQTT Broker.
 *                                      HTTP
 *                                      - HTTP GET command for the Job.
 *                                      - Received the Job from the HTTP server.
 *                                      - HTTP GET command for data.
 *                                      - HTTP PUT command to the report result to the HTTP server (Not implemented yet).
 */

typedef enum
{
    CY_OTA_CB_RSLT_OTA_CONTINUE = 0,  /**< OTA Agent to continue with the function, using the modified data from the application. */
    CY_OTA_CB_RSLT_OTA_STOP,          /**< OTA Agent to end the current update session (do not quit the OTA Agent).               */
    CY_OTA_CB_RSLT_APP_SUCCESS,       /**< Application completed task; OTA Agent uses success.                                    */
    CY_OTA_CB_RSLT_APP_FAILED,        /**< Application failed task; OTA Agent uses failure.                                       */

    CY_OTA_CB_NUM_RESULTS             /**< Placeholder, do not use. */
} cy_ota_callback_results_t;

/** \} group_ota_typedefs */

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/**
 * \addtogroup group_ota_structures
 * \{
 *  Structures used for network connection and OTA Agent behavior.
 */


/**
 * @brief Struct to hold information on where to write the data.
 * Information on where to store the downloaded chunk of the OTA image.
 * \struct cy_ota_storage_write_info_t
 */
typedef struct
{
    uint32_t        total_size;     /**< Pass the total size to the storage module; 0 = disregard.  */

    uint32_t        offset;         /**< Offset into file/area where the data belongs.              */
    uint8_t         *buffer;        /**< Pointer to the buffer with the chunk of data.              */
    uint32_t        size;           /**< Size of the data in the buffer.                            */

                                    /* When using MQTT connection.                                  */
    uint16_t        packet_number;  /**< MQTT: The packet number of this chunk.                     */
    uint16_t        total_packets;  /**< MQTT: Total packets (chunks) in the OTA image.             */
} cy_ota_storage_write_info_t;

#ifdef COMPONENT_OTA_HTTP
/**
 * @brief OTA HTTP-specific connection parameters.
 * \struct cy_ota_http_params_t
 */
typedef struct
{
    cy_awsport_server_info_t        server;     /**< HTTP server to get the Job or OTA image.
                                                 *    Set use_get_job_flow in cy_ota_network_params_t
                                                 *    to determine the use of the Job document.
                                                 */

    const char                      *file;      /**< File name for the Job or OTA image.
                                                 *    Set use_get_job_flow in cy_ota_network_params_t
                                                 *    to determine the OTA update flow.
                                                 */
    cy_awsport_ssl_credentials_t    credentials;/**< Setting credentials uses TLS (NULL == non-TLS). */
} cy_ota_http_params_t;
#endif  /* COMPONENT_OTA_HTTP */

#ifdef COMPONENT_OTA_MQTT

/**
 *  OTA MQTT-specific connection parameters.
 *  \struct cy_ota_mqtt_params_t
 */
typedef struct
{
    bool                        awsIotMqttMode;    /**< 0 = normal MQTT; 1 = Special Amazon mode.    */
    const char                  *pIdentifier;      /**< Pointer to the device ID.                    */
    uint8_t                     numTopicFilters;   /**< Number of topics for MQTT Subscribe.         */
    const char                  **pTopicFilters;   /**< Topic filter text.                           */
    cy_ota_mqtt_session_type_t  session_type;      /**< @ref cy_ota_mqtt_session_type_t              */

    cy_awsport_server_info_t        broker;            /**< Broker to get the Job or OTA image.
                                                    *    Set use_get_job_flow in cy_ota_network_params_t
                                                    *    to determine the OTA update flow.
                                                    */
    cy_awsport_ssl_credentials_t    credentials;     /**< Setting credentials uses TLS (NULL == non-TLS). */

} cy_ota_mqtt_params_t;

#endif  /* COMPONENT_OTA_MQTT   */

/**
 * @brief Structure passed to the callback.
 *
 * This holds read/write data for the application to adjust various things in the callback.
 * The callback is not in an interrupt context, but try to keep it quick!
 * After your function returns, this structure is not available.
 * \struct cy_ota_cb_struct_t
 */
typedef struct
{
    cy_ota_cb_reason_t          reason;         /**< Reason for the callback.                               */
    void                        *cb_arg;        /**< Argument passed when registering the callback.         */

    cy_ota_agent_state_t        ota_agt_state;  /**< Current OTA Agent state.                               */
    cy_rslt_t                   error;          /**< Current OTA Agent error status.                        */

    cy_ota_storage_write_info_t *storage;       /**< Pointer to a chunk of data to write.                   */
    uint32_t                    total_size;     /**< Total # bytes to be downloaded.                        */
    uint32_t                    bytes_written;  /**< Total # bytes downloaded.                              */
    uint32_t                    percentage;     /**< Percentage of bytes downloaded.                        */

    cy_ota_connection_t         connection_type; /**< Connection type @ref cy_ota_connection_t.             */

#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
    cy_awsport_server_info_t    broker_server;   /**< MQTT Broker (or HTTP server) for connection.          */
    cy_awsport_ssl_credentials_t *credentials;     /**< Setting credentials uses TLS (NULL == non-TLS).     */
#endif

#ifdef COMPONENT_OTA_MQTT
    cy_mqtt_t                   mqtt_connection; /**< For Passing MQTT connection instance                  */
    char                        unique_topic[CY_OTA_MQTT_UNIQUE_TOPIC_BUFF_SIZE];   /**< Topic for receiving the OTA data. */
#endif
#ifdef COMPONENT_OTA_HTTP
    cy_http_client_t            http_connection; /**< For Passing HTTP connection instance                  */
#endif

#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
    char                        file[CY_OTA_MQTT_FILENAME_BUFF_SIZE]; /**< File name to request OTA data.   */

    /* For Get Job Message. */
    char                        json_doc[CY_OTA_JSON_DOC_BUFF_SIZE];                /**< Message to request the OTA data.  */
#endif

} cy_ota_cb_struct_t;

/** \} group_ota_structures */


/**
 * \addtogroup group_ota_callback
 * \{
 */

/**
 * @brief OTA Agent callback to the application.
 *
 * The application can use the callback to override default OTA functions.
 *
 * @param[in,out]   cb_data   Current information that Application callback can use/modify.
 *
 * @return
 */
typedef cy_ota_callback_results_t (*cy_ota_callback_t)(cy_ota_cb_struct_t *cb_data);

/** \} group_ota_callback */

/**
 * \addtogroup group_ota_structures
 * \{
 */
/**
 * @brief OTA Connection parameters structure.
 *
 *  This information is used for MQTT and HTTP connections.
 *  \struct cy_ota_network_params_t
 */
typedef struct
{
    cy_ota_connection_t         initial_connection; /**< Initial connection type @ref cy_ota_connection_t.   */

    /* MQTT and HTTP  settings */
#ifdef COMPONENT_OTA_MQTT
    cy_ota_mqtt_params_t        mqtt;               /**< MQTT connection information.                        */
#endif
#ifdef COMPONENT_OTA_HTTP
    cy_ota_http_params_t        http;               /**< HTTP connection information.                        */
#endif
    cy_ota_update_flow_t        use_get_job_flow;   /**< Job flow (CY_OTA_JOB_FLOW or CY_OTA_DIRECT_FLOW).   */
} cy_ota_network_params_t;

/**
 * @brief OTA Agent parameters structure.
 *
 * These parameters are for describing some aspects of the OTA Agent.
 * \struct cy_ota_agent_params_t
 */
typedef struct
{
    uint8_t     reboot_upon_completion;     /**< 1 = Automatically reboot upon download completion and verify.  */
    uint8_t     validate_after_reboot;      /**< 0 = OTA will set MCUboot to permanent before reboot.
                                             *   1 = Application must call cy_ota_validated() after reboot.     */
    bool        do_not_send_result;         /**< If true, do not send the result status.                        */

    cy_ota_callback_t   cb_func;            /**< Notification callback function.                                */
    void                *cb_arg;            /**< Opaque argument passed to the notification callback function.  */
} cy_ota_agent_params_t;

/** \} group_ota_structures */


/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

/**
 * \addtogroup group_ota_functions
 * \{
 * OTA functions for starting, stopping, and getting information about the OTA Agent.
 */

/**
 * @brief Start the OTA Background Agent.
 *
 * Start a thread to monitor for OTA updates.
 * This consumes resources until cy_ota_agent_stop() is called.
 *
 * @param[in]   network_params   Pointer to cy_ota_network_params_t.
 * @param[in]   agent_params     Pointer to cy_ota_agent_params_t.
 * @param[out]  ctx_ptr          Handle to store the OTA Agent context structure pointer,
 *                               Which is used for other OTA calls.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_BADARG
 *          CY_RSLT_OTA_ERROR_ALREADY_STARTED
 *          CY_RSLT_OTA_ERROR
 */
cy_rslt_t cy_ota_agent_start(cy_ota_network_params_t *network_params, cy_ota_agent_params_t *agent_params, cy_ota_context_ptr *ctx_ptr);

/**
 * @brief Stop OTA Background Agent.
 *
 *  Stop the thread to monitor OTA updates and release resources.
 *
 * @param[in]   ctx_ptr         Pointer to the OTA Agent context storage returned from @ref cy_ota_agent_start();
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_BADARG
 */
cy_rslt_t cy_ota_agent_stop(cy_ota_context_ptr *ctx_ptr);

/**
 * @brief Check for OTA update availability and download update now.
 *
 * Once cy_ota_start() has returned CY_RSLT_SUCCESS, you can initiate an OTA update availability check
 * whenever you require; this supercedes the timing values. It does not change the timer values, and will
 * set the timer to the current values after an OTA download cycle.
 *
 *
 * @param[in]   ctx_ptr         Pointer to the OTA Agent context storage returned from @ref cy_ota_agent_start();
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_BADARG
 *          CY_RSLT_OTA_ERROR_ALREADY_STARTED
 */
cy_rslt_t cy_ota_get_update_now(cy_ota_context_ptr ctx_ptr);

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
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_storage_validated(void);

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
/**
 * @brief Set the OTA log output level.
 *
 * To get the OTA log output, first call cy_log_init().
 * Call this routine at any time to set the OTA logging level.
 *
 * @param[in]  level    Required logging level from OTA logs.
 *
 * @result  N/A
 */
void cy_ota_set_log_level(CY_LOG_LEVEL_T level);

/**
 * @brief Get the OTA Agent State.
 *
 * Use this function to determine what state the OTA Agent is in. Do not check if the
 * OTA Agent is in CY_OTA_STATE_AGENT_WAITING because it will interrupt an OTA cycle.
 *
 * @param[in]  ota_ptr          Pointer to the OTA Agent context returned from @ref cy_ota_agent_start();
 * @param[out] ota_state        Current OTA State.
 *
 * @result  CY_RSLT_SUCCESS     sets @ref cy_ota_agent_state_t
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_get_state(cy_ota_context_ptr ota_ptr, cy_ota_agent_state_t *ota_state);

/**
 * @brief Get the last OTA error.
 *
 * The last error value is persistent, and can be queried after @ref cy_ota_agent_stop().
 *
 * @result the last error that occurred (see cy_rslt_t).
 */
cy_rslt_t cy_ota_get_last_error(void);

/**
 * @brief Get a string representation of the error.
 *
 * This can be called at any time.
 *
 * @param[in]   error      Value returned from cy_ota_get_last_error().
 *
 * @result Pointer to a string (NULL for unknown error).
 */
const char *cy_ota_get_error_string(cy_rslt_t error);

/**
 * @brief Get a string Representation of the OTA state.
 *
 * This can be called at any time.
 *
 * @param[in]   state_value      @ref cy_ota_agent_state_t
 *
 * @result  Pointer to the string (NULL for unknown state).
 */
const char *cy_ota_get_state_string(cy_ota_agent_state_t state_value);

/**
 * @brief Get a string representation for the callback reason.
 *
 * This can be called at any time.
 *
 * @param[in]   reason      @ref cy_ota_cb_reason_t
 *
 * @result  Pointer to the string (NULL for unknown reason).
 */
const char *cy_ota_get_callback_reason_string(cy_ota_cb_reason_t reason);

/** \} group_ota_functions */

#ifdef __cplusplus
    }
#endif

#endif /* CY_OTA_API_H__ */

 /** \} group_cy_ota */
